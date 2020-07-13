#include "options.h"
#include "parser.h"
#include "task.h"

#include "heuristics/heuristic.h"
#include "heuristics/heuristic_factory.h"
#include "search_engines/search.h"
#include "search_engines/search_factory.h"
#include "successor_generators/successor_generator.h"
#include "successor_generators/successor_generator_factory.h"
#include "utils/system.h"

#include <iostream>
#include <memory>

using namespace std;
using namespace utils;

int main(int argc, char *argv[]) {
    cout << "Initializing planner" << endl;

    Options opt(argc, argv);

    ifstream in(opt.get_filename());
    if (!in) {
        cerr << "Error opening the task file." << endl;
        exit(-1);
    }

    cout << "Reading task description file." << endl;
    cin.rdbuf(in.rdbuf());

    // Parse file
    string domain_name, task_name;
    cin >> domain_name >> task_name;
    Task task(domain_name, task_name);
    cout << task.get_domain_name() << " " << task.get_task_name() << endl;

    bool parsed = parse(task, in);
    if (!parsed) {
        cerr << "Parser failed." << endl;
        exit(-1);
    }

    cout << "IMPORTANT: Assuming that negative effects are always listed first. "
            "(This is guaranteed by the default translator.)" << endl;

    // Let's create a couple unique_ptr's that deal with mem allocation themselves
    std::unique_ptr<SearchBase> search(SearchFactory::create(opt.get_search_engine(), opt.get_state_representation()));
    std::unique_ptr<Heuristic> heuristic(HeuristicFactory::create(opt.get_evaluator(), task));
    std::unique_ptr<SuccessorGenerator> sgen(SuccessorGeneratorFactory::create(opt.get_successor_generator(),
                                                                               opt.get_seed(),
                                                                               task));

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
        search->print_statistics();
        exit_with(utils::ExitCode::SEARCH_OUT_OF_MEMORY);
    }

}
