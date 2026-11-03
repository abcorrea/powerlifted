#include "options.h"
#include "parser.h"
#include "plan_manager.h"
#include "task.h"

#include "grounding/ground_task.h"
#include "heuristics/ground_delete_relaxation_heuristic.h"
#include "heuristics/heuristic.h"
#include "heuristics/heuristic_factory.h"
#include "search_engines/ground_search.h"
#include "search_engines/search.h"
#include "search_engines/search_factory.h"
#include "successor_generators/successor_generator.h"
#include "successor_generators/successor_generator_factory.h"
#include "utils/system.h"

#include <iostream>
#include <memory>
#include <optional>
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
      With a delete-relaxation heuristic (add, hmax, ff), ground the task. If
      grounding succeeds, alt-bfws1, alt-bfws2, and bfws2-lmnov search on the
      ground task; otherwise, they fall back to lifted search. The
      delete-relaxation heuristics use the ground task (or its decomposed
      relaxed program, if there are too many ground operators) also in
      lifted search.
     */
    const std::string &engine = opt.get_search_engine();
    bool ground_engine = engine == "alt-bfws1" || engine == "alt-bfws2" || engine == "bfws2-lmnov";
    std::optional<GroundDeleteRelaxationHeuristic::Type> heuristic_type =
        GroundDeleteRelaxationHeuristic::get_type(opt.get_evaluator());
    if (!opt.get_force_lifted() && heuristic_type) {
        task.set_ground_task(GroundTask::create(
            task, opt.get_max_ground_operators(), opt.get_grounding_time_limit()));
    }
    const std::shared_ptr<GroundTask> &ground_task = task.get_ground_task();
    if (ground_engine && ground_task && !ground_task->relaxed_program_only) {
        PlanManager::set_plan_filename(opt.get_plan_file());
        try {
            // bfws2-lmnov: width-2 novelty partitioned by the landmark count
            // (as in NOLAN), plus landmark-count and lazy novelty open lists.
            GroundSearchOptions search_options;
            search_options.heuristic_type = *heuristic_type;
            search_options.width = engine == "alt-bfws1" ? 1 : 2;
            search_options.use_landmarks = engine == "bfws2-lmnov";
            search_options.full_novelty_check = opt.use_full_novelty_check();
            if (engine == "bfws2-lmnov") {
                // Type-based exploration list, greedy boosts (300 pops of the
                // preferred heuristic and landmark lists) after heuristic
                // improvements, and width 2 only while its checks are cheap.
                search_options.type_based = true;
                search_options.greedy_boost = true;
                search_options.preferred_boost = 300;
                search_options.max_tuples_per_check = 5000;
            }
            // TODO: experimental variants, to be removed.
            const std::string &experimental = "," + opt.get_experimental() + ",";
            auto has = [&](const std::string &name) {
                return experimental.find("," + name + ",") != std::string::npos;
            };
            search_options.type_based = search_options.type_based || has("tb");
            search_options.progress_novelty = has("pn") || has("pn1");
            if (has("pn1"))
                search_options.progress_novelty_width = 1;
            search_options.accept_landmarks_without_predecessors = has("lmpast");
            search_options.helpful_actions = has("ha");
            search_options.greedy_boost =
                search_options.greedy_boost || has("gb") || has("gbt") || has("gbn");
            search_options.boost_type_based = has("gbt");
            search_options.boost_novelty = has("gbn");
            size_t boost = experimental.find(",boost");
            if (boost != std::string::npos)
                search_options.preferred_boost = std::stoi(experimental.substr(boost + 6));
            size_t tpc = experimental.find(",tpc");
            if (tpc != std::string::npos)
                search_options.max_tuples_per_check = std::stoll(experimental.substr(tpc + 4));
            if (has("w3"))
                search_options.width = 3;
            GroundSearch search(task, ground_task, search_options);
            auto exitcode = search.search();
            search.print_statistics();
            utils::report_exit_code_reentrant(exitcode);
            return static_cast<int>(exitcode);
        }
        catch (const bad_alloc &ex) {
            exit_with(utils::ExitCode::SEARCH_OUT_OF_MEMORY);
        }
    }

    // Let's create a couple unique_ptr's that deal with mem allocation themselves
    // Lifted width-2 novelty is expensive: if the task could not be ground,
    // search with width 1 unless lifted search was requested explicitly.
    std::string lifted_engine = opt.get_search_engine();
    if ((lifted_engine == "alt-bfws2" || lifted_engine == "bfws2-lmnov") && !opt.get_force_lifted())
        lifted_engine = "alt-bfws1";
    std::unique_ptr<SearchBase> search(SearchFactory::create(opt, lifted_engine));
    std::unique_ptr<Heuristic> heuristic(HeuristicFactory::create(opt, task));
    std::unique_ptr<SuccessorGenerator> sgen(
        SuccessorGeneratorFactory::create(opt.get_successor_generator(), opt.get_seed(), task));

    PlanManager::set_plan_filename(opt.get_plan_file());

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
