#ifndef SEARCH_DUMP_TASK_TO_PDDL_H
#define SEARCH_DUMP_TASK_TO_PDDL_H
#include "task.h"
#include <string>

constexpr const char* domainName=nullptr;

void dumpToPDDLDomain(const Task& task, const std::string& domainFileName);
void dumpToPDDLProblem(const DBState& initialState, const Task& task, const std::string& problemFileName);


#endif // SEARCH_DUMP_TASK_TO_PDDL_H