
#include "table.h"

const Table &Table::EMPTY_TABLE() {
    static Table table; // This will be initialized only on the first call
    return table;
}