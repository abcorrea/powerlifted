#include "ordered_join_successor.h"
#include "../action.h"
#include "../database/table.h"
#include "../states/state.h"

#include <vector>

using namespace std;

template <typename OrderT>
bool OrderedJoinSuccessorGenerator<OrderT>::parse_precond_into_join_program(
    const PrecompiledActionData &adata, const DBState &state, std::vector<Table>& tables)
{
    // TODO This is highly inefficient. See issue #6:
    //      https://github.com/abcorrea/powerlifted/issues/6
    bool res = GenericJoinSuccessor::parse_precond_into_join_program(adata, state, tables);
    if (!res) return false;

    std::sort(tables.begin(), tables.end(), OrderT());
    return true;
}

template <typename OrderT>
OrderedJoinSuccessorGenerator<OrderT>::OrderedJoinSuccessorGenerator(const Task &task)
    : GenericJoinSuccessor(task)
{
}

// explicit template instantiations
template class OrderedJoinSuccessorGenerator<OrderTable>;
template class OrderedJoinSuccessorGenerator<InverseOrderTable>;