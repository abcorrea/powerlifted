# Power Lifted Planner

Implementation of a lifted planner using database techniques for successor
generator.

(See [References](#references) for more details)

## Usage

The `powerlifted.py` script solves a PDDL task provided as input. It also builds
the planner if the `--build` parameter is passed. The script has the following
parameters:

```powerlifted.py [-d DOMAIN] -i INSTANCE -s SEARCH -e HEURISTIC -g GENERATOR [--build] [--state STATE REPR.]```

Use the `build.py` script to build the planner first.

### Available Options for SEARCH:
- `naive`: Breadth-First Search
- `gbfs`: Greedy Best-First Search

### Available Options for HEURISTIC:
- `blind`: No Heuristic
- `goalcount`: The goal-count/STRIPS heuristic

### Available Options for GENERATOR:
- `join`: Join program using the predicate order given in the PDDL file
- `random_join`: Randomly ordered join program
- `ordered_join`: Join program ordered by the arity of the predicates
- `full_reducer`: Generate successor for acyclic schemas using the full
  reducer method; for cyclic schemas it uses a partial reducer and a join
  program.
- `yannakakis`: Same as above but replaces the final join of the full
      reducer method by the Yannakakis' project-join program.

### Available Options for STATE REPR.:

- `sparse`: Use the sparse state representation where a state is only
  represented by the facts that are true in this state.
- `extensional`: Use the extensional representation where a state is a bitset
  where the ith-bit is true if the fact associated to it is true in this
  state. This representation requires the grounding of facts (but not of
  actions) which, right now, is performed in the search component.

## Components
 - Translator
 - Search component

## Requirements
 - C++ 17
 - CMake 3.12+
 - Python 3.5+

## Limitations
 - **Axioms**: not supported
 - **Conditional effects**: not supported
 - **Costs**: ignored
 - **Negated preconditions**: only inequality
 - **Quantifiers**: not supported

 ## References

 1. Corrêa, A. B.; Pommerening, F.; Helmert, M.; and Francès, G. 2020. Lifted Successor Generation using Query Optimization Techniques. In Proc. ICAPS 2020. (To appear) [[pdf]](https://ai.dmi.unibas.ch/papers/correa-et-al-icaps2020.pdf)
 2. Corrêa, A. B.; Pommerening, F.; Helmert, M.; and Francès, G. 2020. Code from the paper "Lifted Successor Generationusing Query Optimization Techniques".  https://doi.org/10.5281/zenodo.3687008
