#include "random_successor.h"

#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

RandomSuccessorGenerator::RandomSuccessorGenerator(const Task &task) : GenericJoinSuccessor(task) {
    srand(time(nullptr));
}

vector<Table> RandomSuccessorGenerator::parse_precond_into_join_program(
    const vector<Atom> &precond,
    const DBState &state)
{
    // We just shuffle the result of the standard successor generator
    auto result = GenericJoinSuccessor::parse_precond_into_join_program(precond, state);
    shuffle(result.begin(), result.end(), std::default_random_engine(rand()));
    return result;
}