#include "random_successor.h"

#include "../database/table.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

using namespace std;

RandomSuccessorGenerator::RandomSuccessorGenerator(const Task &task, unsigned seed) :
    GenericJoinSuccessor(task),
    rng(seed)
{
}

vector<Table> RandomSuccessorGenerator::parse_precond_into_join_program(
    const vector<Atom> &precond,
    const DBState &state)
{
    // We just shuffle the result of the standard successor generator
    auto result = GenericJoinSuccessor::parse_precond_into_join_program(precond, state);
    shuffle(result.begin(), result.end(), rng);
    return result;
}