# Power Lifted

Implementation of a lifted planner using database techniques for successor
generator.

## Usage
The `power-lifted.py` script compiles and solves a PDDL task provided as
input. The script has the following parameters:

```power-lifted.py -d DOMAIN -i INSTANCE -s SEARCH -e HEURISTIC -g GENERATOR```

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

## Components
 - Translator
 - Search component

## Requirements
 - C++ 17
 - Python 3.5+

## Limitations
 - **Axioms**: not supported
 - **Conditional effects**: not supported
 - **Costs**: ignored
 - **Negated preconditions**: only inequality
 - **Quantifiers**: not supported
