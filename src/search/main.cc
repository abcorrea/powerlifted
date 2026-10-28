#include "options.h"
#include "parser.h"
#include "plan_manager.h"
#include "task.h"

#include "grounded/grounded_search.h"
#include "grounding/ground_task.h"
#include "heuristics/heuristic.h"
#include "heuristics/heuristic_factory.h"
#include "search_engines/search.h"
#include "search_engines/search_factory.h"
#include "successor_generators/successor_generator.h"
#include "successor_generators/successor_generator_factory.h"
#include "utils/system.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>

using namespace std;
using namespace utils;

int main(int argc, char *argv[])
{
    cout << "Initializing planner" << endl;

    Options opt(argc, argv);

    ifstream task_file(opt.get_filename());
    if (!task_file) {
        cerr << "Error opening the task file: " << opt.get_filename() << endl;
        exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }

    cout << "Reading task description file." << endl;
    cin.rdbuf(task_file.rdbuf());

    // Parse file
    string domain_name, task_name;
    cin >> domain_name >> task_name;
    Task task(domain_name, task_name);
    cout << task.get_domain_name() << " " << task.get_task_name() << endl;

    bool parsed = parse(task, task_file);
    if (!parsed) {
        cerr << "Parser failed." << endl;
        exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }

    /*
      We check here that if the task has object creation effects, then the configuration
      used actually supports it.
     */
    if (task.has_object_creation()) {
        bool unsupported_features = false;
        std::set<std::string> OBJ_CREATION_SEARCH = {
            "astar", "bfs", "bfws1", "bfws2", "iw1", "iw2", "gbfs", "lazy"};
        std::string search_engine = opt.get_search_engine();
        if (!OBJ_CREATION_SEARCH.count(search_engine)) {
            std::cerr << "ERROR: Search algorithm " << search_engine
                      << " does not support object creation." << std::endl;
            std::cerr << "Search engines supporting object creation:" << std::endl;
            for (const string &s : OBJ_CREATION_SEARCH) {
                std::cerr << "\t- " << s << std::endl;
            }
            unsupported_features = true;
        }
        std::set<std::string> OBJ_CREATION_EVALUATOR = {"blind", "goalcount"};
        std::string evaluator = opt.get_evaluator();
        if (!OBJ_CREATION_EVALUATOR.count(evaluator)) {
            std::cerr << "ERROR: Evaluator/heuristic " << evaluator
                      << " does not support object creation." << std::endl;
            std::cerr << "Heuristics/evaluators supporting object creation:" << std::endl;
            for (const string &s : OBJ_CREATION_EVALUATOR) {
                std::cerr << "\t- " << s << std::endl;
            }
            unsupported_features = true;
        }
        std::set<std::string> OBJ_CREATION_SUCCESSOR = {
            "full_reducer", "join", "ordered_join", "random_join", "yannakakis"};
        std::string successor_generator = opt.get_successor_generator();
        if (!OBJ_CREATION_SUCCESSOR.count(successor_generator)) {
            std::cerr << "ERROR: Successor generator " << successor_generator
                      << " does not support object creation." << std::endl;
            std::cerr << "Successor generators supporting object creation:" << std::endl;
            for (const string &s : OBJ_CREATION_SUCCESSOR) {
                std::cerr << "\t- " << s << std::endl;
            }
            unsupported_features = true;
        }
        if (unsupported_features) {
            std::cerr << "ABORTING: Some of the used features do not support object creation."
                      << std::endl;
            exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
        }
    }

    cout << "IMPORTANT: Assuming that negative effects are always listed first. "
            "(This is guaranteed by the default translator.)"
         << endl;

    if (task.is_trivially_unsolvable()) {
        cout << "Problem goal was statically determined to be unsatisfiable." << endl;
        exit_with(utils::ExitCode::SEARCH_UNSOLVABLE);
    }

    /*
      If the task can be grounded, search on the ground representation
      (alt-bfws1 only); otherwise fall back to lifted search.
     */
    if (opt.get_force_lifted())
        GroundTask::disable_grounding();
    const std::string &engine = opt.get_search_engine();
    bool grounded_engine = engine == "alt-bfws1" || engine == "alt-bfws2" ||
        engine == "lama" || engine == "alt-bfws1-lm" || engine == "alt-bfws2-lm" ||
        engine == "portfolio" || engine == "dual-bfws" || engine == "bfws2-lmnov";
    if (grounded_engine && !std::getenv("PWL_LIFTED_SEARCH")) {
        try {
            std::shared_ptr<GroundTask> ground_task = GroundTask::get_cached(task);
            if (ground_task) {
                PlanManager::set_plan_filename(opt.get_plan_file());
                vector<GroundSearchConfig> configs;
                auto make_config = [&](const std::string &name) {
                    GroundSearchConfig c;
                    c.name = name;
                    c.novelty = name != "lama";
                    c.lm = name == "lama" || name == "alt-bfws1-lm" || name == "alt-bfws2-lm";
                    c.width = (name == "alt-bfws2" || name == "alt-bfws2-lm" ||
                               name == "bfws2-lmnov") ? 2 : 1;
                    if (name == "bfws2-lmnov") {
                        // NOLAN-like: width-2 novelty partitioned by the
                        // landmark count (eager and lazy), plus h-FF and
                        // landmark queues.
                        c.lm = true;
                        c.novelty_by_lm = true;
                        c.lazy_queue = !std::getenv("PWL_NO_LAZY_QUEUE");
                    }
                    return c;
                };
                if (engine == "dual-bfws") {
                    // 1-BFWS (novelty > 1 pruned) for a short time, then
                    // alternated BFWS.
                    GroundSearchConfig pruned = make_config("alt-bfws1");
                    pruned.name = "alt-bfws1-pruned";
                    pruned.prune_novelty = 1;
                    pruned.max_seconds = 10;
                    configs.push_back(pruned);
                    configs.push_back(make_config("alt-bfws1"));
                }
                else if (engine == "portfolio" && std::getenv("PWL_PORTFOLIO")) {
                    // Comma-separated engine names; ":sN" shuffles the
                    // successor order with seed N, ":add" adds an h-add queue,
                    // ":lazy" uses lazy novelty, ":tN" starts the run after N
                    // seconds.
                    std::string spec = std::getenv("PWL_PORTFOLIO");
                    size_t pos = 0;
                    while (pos <= spec.size()) {
                        size_t next = spec.find(',', pos);
                        if (next == std::string::npos)
                            next = spec.size();
                        std::string item = spec.substr(pos, next - pos);
                        pos = next + 1;
                        if (item.empty())
                            continue;
                        std::string name = item.substr(0, item.find(':'));
                        GroundSearchConfig c = make_config(name);
                        c.name = item;
                        size_t opt_pos = item.find(':');
                        while (opt_pos != std::string::npos) {
                            size_t opt_end = item.find(':', opt_pos + 1);
                            std::string o = item.substr(opt_pos + 1, opt_end == std::string::npos
                                                        ? std::string::npos : opt_end - opt_pos - 1);
                            if (!o.empty() && o[0] == 's') {
                                c.shuffle = true;
                                c.seed = std::atoi(o.c_str() + 1);
                            }
                            else if (o == "add") {
                                c.add = true;
                            }
                            else if (o == "lazy") {
                                c.lazy_novelty = true;
                            }
                            else if (o == "lazyq") {
                                c.lazy_queue = true;
                            }
                            else if (!o.empty() && o[0] == 't') {
                                c.start_time = std::atof(o.c_str() + 1);
                            }
                            opt_pos = opt_end;
                        }
                        configs.push_back(c);
                    }
                }
                else if (engine == "portfolio") {
                    configs.push_back(make_config("alt-bfws2"));
                    GroundSearchConfig add_config = make_config("alt-bfws1");
                    add_config.name = "alt-bfws1-add";
                    add_config.add = true;
                    configs.push_back(add_config);
                    configs.push_back(make_config("lama"));
                }
                else {
                    configs.push_back(make_config(engine));
                }
                const std::string &queues = opt.get_grounded_queues();
                if (!queues.empty()) {
                    auto has = [&](const std::string &q) {
                        return ("," + queues + ",").find("," + q + ",") != std::string::npos;
                    };
                    for (GroundSearchConfig &c : configs) {
                        c.novelty = has("novelty");
                        c.ff = has("ff");
                        c.add = has("add");
                        c.lm = has("lm");
                    }
                }
                GroundedSearch search(task, ground_task, opt.get_evaluator(),
                                      opt.use_full_novelty_check());
                auto exitcode = search.search(configs);
                search.print_statistics();
                utils::report_exit_code_reentrant(exitcode);
                return static_cast<int>(exitcode);
            }
        }
        catch (const bad_alloc &ex) {
            exit_with(utils::ExitCode::SEARCH_OUT_OF_MEMORY);
        }
    }

    // Let's create a couple unique_ptr's that deal with mem allocation themselves
    // Without grounding, use width 1 unless lifted search was requested.
    std::string lifted_engine = opt.get_search_engine();
    if ((lifted_engine == "alt-bfws2" || lifted_engine == "bfws2-lmnov") &&
        !opt.get_force_lifted())
        lifted_engine = "alt-bfws1";
    std::unique_ptr<SearchBase> search(SearchFactory::create(opt, lifted_engine));
    std::unique_ptr<Heuristic> heuristic(HeuristicFactory::create(opt, task));
    std::unique_ptr<SuccessorGenerator> sgen(
        SuccessorGeneratorFactory::create(opt.get_successor_generator(), opt.get_seed(), task));

    PlanManager::set_plan_filename(opt.get_plan_file());

    // Start search
    if (task.is_trivially_unsolvable()) {
        cout << "Problem goal was statically determined to be unsatisfiable." << endl;
        exit_with(utils::ExitCode::SEARCH_UNSOLVABLE);
    }

    try {
        auto exitcode = search->search(task, *sgen, *heuristic);
        search->print_statistics();
        heuristic->print_statistics();
        utils::report_exit_code_reentrant(exitcode);
        return static_cast<int>(exitcode);
    }
    catch (const bad_alloc &ex) {
        // search->print_statistics();
        exit_with(utils::ExitCode::SEARCH_OUT_OF_MEMORY);
    }
}
