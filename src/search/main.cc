#include "options.h"
#include "parser.h"
#include "plan_manager.h"
#include "task.h"

#include "heuristics/heuristic.h"
#include "heuristics/heuristic_factory.h"
#include "search_engines/search.h"
#include "search_engines/search_factory.h"
#include "successor_generators/successor_generator.h"
#include "successor_generators/successor_generator_factory.h"

#include <iostream>
#include <memory>

using namespace std;
using namespace utils;

int main(int argc, char *argv[]) {
    cout << "Initializing planner" << endl;

    Options opt(argc, argv);

    ifstream task_file(opt.get_filename());
    if (!task_file) {
        cerr << "Error opening the task file: " << opt.get_filename() << endl;
        exit(-1);
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
        exit(-1);
    }

    /*
      We check here that if the task has object creation effects, then the configuration
      used actually supports it.
     */
    if (task.has_object_creation()) {
      bool unsupported_features = false;
      std::set<std::string> OBJ_CREATION_SEARCH = {"astar", "bfs", "bfws1", "bfws2", "iw1", "iw2", "gbfs", "lazy"};
      std::string search_engine = opt.get_search_engine();
      if (!OBJ_CREATION_SEARCH.count(search_engine)) {
        std::cerr << "ERROR: Search algorithm " << search_engine << " does not support object creation." << std::endl;
        std::cerr << "Search engines supporting object creation:" << std::endl;
        for (const string &s : OBJ_CREATION_SEARCH) {
          std::cerr << "\t- " << s << std::endl;
        }
        unsupported_features = true;
      }
      std::set<std::string> OBJ_CREATION_EVALUATOR = {"blind", "goalcount"};
      std::string evaluator = opt.get_evaluator();
      if (!OBJ_CREATION_EVALUATOR.count(evaluator)) {
        std::cerr << "ERROR: Evaluator/heuristic " << evaluator << " does not support object creation." << std::endl;
        std::cerr << "Heuristics/evaluators supporting object creation:" << std::endl;
        for (const string &s : OBJ_CREATION_EVALUATOR) {
          std::cerr << "\t- " << s << std::endl;
        }
        unsupported_features = true;
      }
      std::set<std::string> OBJ_CREATION_SUCCESSOR = {"full_reducer",  "join", "ordered_join", "random_join", "yannakakis"};
      std::string successor_generator = opt.get_successor_generator();
      if (!OBJ_CREATION_SUCCESSOR.count(successor_generator)) {
        std::cerr << "ERROR: Successor generator " << successor_generator << " does not support object creation." << std::endl;
        std::cerr << "Successor generators supporting object creation:" << std::endl;
        for (const string &s : OBJ_CREATION_SUCCESSOR) {
          std::cerr << "\t- " << s << std::endl;
        }
        unsupported_features = true;
      }
      if (unsupported_features) {
        std::cerr << "ABORTING: Some of the used features do not support object creation." << std::endl;
        exit(-1);
      }
    }

    cout << "IMPORTANT: Assuming that negative effects are always listed first. "
            "(This is guaranteed by the default translator.)" << endl;

    // Let's create a couple unique_ptr's that deal with mem allocation themselves
    std::unique_ptr<SearchBase> search(SearchFactory::create(opt, opt.get_search_engine()));
    std::unique_ptr<Heuristic> heuristic(HeuristicFactory::create(opt, task));
    std::unique_ptr<SuccessorGenerator> sgen(SuccessorGeneratorFactory::create(opt.get_successor_generator(),
                                                                               opt.get_seed(),
                                                                               task));

    PlanManager::set_plan_filename(opt.get_plan_file());

    // Start search
    if (task.is_trivially_unsolvable()) {
        cout << "Problem goal was statically determined to be unsatisfiable." << endl;
        exit_with(utils::ExitCode::SEARCH_UNSOLVABLE);
    }

    try {
        auto exitcode = search->search(task, *sgen, *heuristic);
        search->print_statistics();
        utils::report_exit_code_reentrant(exitcode);
        return static_cast<int>(exitcode);
    }
    catch (const bad_alloc& ex) {
        //search->print_statistics();
        exit_with(utils::ExitCode::SEARCH_OUT_OF_MEMORY);
    }

}
