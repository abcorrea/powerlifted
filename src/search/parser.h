#ifndef SEARCH_PARSER_H
#define SEARCH_PARSER_H

#include "task.h"

#include <fstream>

bool parse(Task &task, const std::ifstream &in);
void output_error(string &msg);

#endif // SEARCH_PARSER_H
