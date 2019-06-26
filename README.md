# Lifted Planning

Implementation of a lifted planner using database techniques for successor
generator.

## Components
 - Translator: part of the code from Fast Downward translator.

## Limitations
 - **Axioms**: not supported
 - **Conditional effects**: not supported
 - **Costs**: ignored
 - **Negated preconditions**: only inequality (possible toextend to negated
   constants as well)
 - **Quantiied Precondition**: not supported

#### Domains excluded:
   - DataNetwork, Snake, Termes, Tetris, TidyBot: negated preconditions
   - Agricola: negated constants
   - Spider: conditional effects
   - PetriNets, Trucks: pre-grounded action schemas

## Requirements

### Paths, etc.

  - Add the folder `planner/reports/utils/` to the `PYTHONPATH` variable  

### Other programs

   - Tarski: a31c455
