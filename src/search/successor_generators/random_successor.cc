#include "random_successor.h"

#include "../database/table.h"

#include <algorithm>
#include <vector>

using namespace std;

RandomSuccessorGenerator::RandomSuccessorGenerator(const Task &task, unsigned seed) :
    GenericJoinSuccessor(task),
    rng(seed)
{
}

bool RandomSuccessorGenerator::parse_precond_into_join_program(
    const PrecompiledActionData &adata, const DBState &state, std::vector<Table>& tables)
{
    bool res = GenericJoinSuccessor::parse_precond_into_join_program(adata, state, tables);
    if (!res) return false;

    shuffle(tables.begin(), tables.end(), rng);
    return true;
}