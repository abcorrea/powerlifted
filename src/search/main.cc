#include "parser.h"
#include "task.h"
#include "utils.h"

#include "heuristics/heuristic.h"
#include "heuristics/heuristic_factory.h"
#include "search_engines/search.h"
#include "search_engines/search_factory.h"
#include "successor_generators/successor_generator.h"
#include "successor_generators/successor_generator_factory.h"

#include <iostream>
#include <memory>

using namespace std;

int main(int argc, char *argv[]) {
    cout << "Initializing planner" << endl;

    if (argc != 6) {
        cerr << "Usage: ./planner [TASK INPUT] [SEARCH METHOD] [HEURISTIC] [SUCCESSOR GENERATOR] [STATE REPRESENTATION]"
             << endl;
        exit(-1);
    }

    ifstream in(argv[1]);
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
    std::unique_ptr<SearchBase> search(SearchFactory::create(argv[2], argv[5]));
    std::unique_ptr<Heuristic> heuristic(HeuristicFactory::create(argv[3], task));
    std::unique_ptr<SuccessorGenerator> sgen(SuccessorGeneratorFactory::create(argv[4], task));

    // Start search
    if (task.is_trivially_unsolvable()) {
        cout << "Goal condition has static information which is not satisfied in the initial state."
             << endl;
        return 0;
    }

    int result = search->search(task, *sgen, *heuristic);

    if (result == NOT_SOLVED) {
        cerr << "State space completely explored and no solution found!" << endl;
    }

    search->print_statistics();

    cout << "Peak memory usage: " << get_peak_memory_in_kb() << " kB\n";
    return 0;
}
