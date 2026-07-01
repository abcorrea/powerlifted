#ifndef SEARCH_DATALOG_GROUNDER_GROUNDER_STATISTICS_H_
#define SEARCH_DATALOG_GROUNDER_GROUNDER_STATISTICS_H_

/*
 * Harness instrumentation for the grounder-work autoresearch loop (NOT a planner
 * feature). The weighted grounder is re-run once per state by the add/ff/...
 * heuristics; these cumulative counters expose its total work across a whole
 * search as a deterministic, noise-free secondary signal next to wall-clock
 * time, so a real work cut (fewer facts produced) can be told apart from a
 * cheaper-per-fact cut (same count, less time) and from a degenerate one (a
 * less-informative heuristic that just expands more states).
 *
 * This file and its call sites (record_grounder_run in weighted_grounder.cc and
 * print_grounder_statistics in main.cc) are part of the measurement harness:
 * the autoresearch agent must NOT edit the counting itself (it is "the meter").
 * They are advisory only -- the primary metric and the correctness gate are
 * computed externally by benchmarks/run_suite.py.
 *
 * C++17 inline variables keep this header-only, so no source file or CMake
 * target list needs to change.
 */

#include <iostream>

namespace datalog {

inline long long grounder_total_atoms_produced = 0;
inline long long grounder_total_queue_pushes = 0;
inline long long grounder_total_calls = 0;

inline void record_grounder_run(long long atoms_produced, long long queue_pushes)
{
    grounder_total_atoms_produced += atoms_produced;
    grounder_total_queue_pushes += queue_pushes;
    ++grounder_total_calls;
}

inline void print_grounder_statistics()
{
    std::cout << grounder_total_atoms_produced
              << " grounder atoms produced (cumulative)" << std::endl;
    std::cout << grounder_total_queue_pushes
              << " grounder queue pushes (cumulative)" << std::endl;
    std::cout << grounder_total_calls
              << " grounder calls (cumulative)" << std::endl;
}

}  // namespace datalog

#endif  // SEARCH_DATALOG_GROUNDER_GROUNDER_STATISTICS_H_
