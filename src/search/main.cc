#include <iostream>
#include <vector>
#include <fstream>

#include "parser.h"
#include "task.h"
#include "utils.h"

#include "search_engines/search_factory.h"
#include "search_engines/search.h"
#include "search_engines/breadth_first_search.h"

#include "successor_generators/successor_generator_factory.h"
#include "successor_generators/successor_generator.h"

#include "heuristics/heuristic_factory.h"
#include "heuristics/heuristic.h"

using namespace std;

int main(int argc, char *argv[]) {
    cout << "Initializing planner" << endl;

    if (argc != 5) {
        cerr << "Usage: ./planner [TASK INPUT] [SEARCH METHOD] [HEURISTIC] [SUCCESSOR GENERATOR]" << endl;
        exit(-1);
    }

    // Remember to change it when it is not debugging anymore
    cout << argv[1] << endl;
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
    cout << task.getDomainName() << " " << task.getTaskName() << endl;

    bool parsed = parse(task, in);
    if (!parsed) {
        cerr << "Parser failed." << endl;
        return -1;
    }

    cout << "IMPORTANT: Search component assumes that negative effects are always listed first." << endl;

    Search *search = SearchFactory::new_search_engine(argv[2]);
    if (!search) {
        cerr << "Invalid search method." << endl;
        return -1;
    }

    Heuristic *heuristic = HeuristicFactory::new_heuristic(argv[3], task);
    if (!heuristic) {
        cerr << "Invalid heuristic." << endl;
        return -1;
    }

    SuccessorGenerator *successorGenerator = SuccessorGeneratorFactory::new_generator(argv[4], task);
    if (!successorGenerator) {
        cerr << "Invalid successor generator method." << endl;
        return -1;
    }


    // Start search
    if (task.is_trivially_unsolvable()) {
        cout << "Goal condition has static information which is not satisfied in the initial state." << endl;
        return -1;
    }

    int result = search->search(task, successorGenerator, *heuristic);

    if (result == -1) {
      cerr << "State space completely explored and no solution found!" << endl;
    }
		       

    cout << "Peak memory usage: " << get_peak_memory_in_kb() << " kB\n";
    return result;
}
