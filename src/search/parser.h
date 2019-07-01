#ifndef SEARCH_PARSER_H
#define SEARCH_PARSER_H

#include <fstream>

#include "task.h"


bool parse(Task &task, const std::ifstream &in);

#endif //SEARCH_PARSER_H
