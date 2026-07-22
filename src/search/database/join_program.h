#ifndef DATABASE_JOIN_PROGRAM_H
#define DATABASE_JOIN_PROGRAM_H

#include "table.h"

#include "../atom.h"
#include "../states/state.h"

#include <vector>

/**
 * @brief A precompiled join program for a conjunctive query over the fluent
 * relations of a state plus the static information.
 *
 * Used both for action-schema preconditions (successor generation) and for
 * axiom bodies (derived-predicate evaluation). Tables of static atoms are
 * precompiled once; tables of fluent atoms are (re)filled from the state
 * being evaluated.
 *
 * @var is_ground: Whether the query has no parameters at all (only set and
 * used by the action-schema path, which checks ground actions separately).
 * @var statically_inapplicable: Whether some static atom of the query has an
 * empty extension, in which case the query can never have an answer.
 * @var relevant_atoms: The (positive, non-nullary, non-'=') atoms of the
 * query, each corresponding to one table of the join program.
 * @var fluent_tables: Indices into relevant_atoms whose tables must be
 * created from each state.
 * @var precompiled_db: The tables of the join program, with the static ones
 * already filled in.
 */
class PrecompiledJoinProgram {
public:
    PrecompiledJoinProgram()
        : is_ground(false), statically_inapplicable(false),
          relevant_atoms(), fluent_tables(), precompiled_db()
    {}

    bool is_ground;
    bool statically_inapplicable;
    std::vector<Atom> relevant_atoms;
    std::vector<unsigned> fluent_tables;
    std::vector<Table> precompiled_db;
};

namespace join_program {

/**
 * Compute, for atom `a`, the tuple index of each argument position (the
 * parameter index for free variables, an always-unique negative number for
 * constants) and the argument positions holding constants.
 */
void get_indices_and_constants(std::vector<int> &indices,
                               std::vector<int> &constants,
                               const Atom &a);

/**
 * Select the tuples of the relation of `a` in `s` matching the constants of
 * the (partially ground) atom `a`.
 */
void select_tuples(const DBState &s,
                   const Atom &a,
                   std::vector<GroundAtom> &tuples,
                   const std::vector<int> &constants);

/**
 * Precompile the join program for the given query atoms. `is_static` says,
 * per predicate index, whether the predicate's extension lives in
 * `static_information` (otherwise its table is refilled from each state).
 */
PrecompiledJoinProgram precompile(std::vector<Atom> &&atoms,
                                  const std::vector<bool> &is_static,
                                  const StaticInformation &static_information);

/**
 * Create the tables of the join program for the given state: copies the
 * precompiled static tables and fills the fluent ones from `state`.
 *
 * @return false if some table is empty (the query has no answer).
 */
bool fill_tables(const PrecompiledJoinProgram &program,
                 const DBState &state,
                 std::vector<Table> &tables);

/**
 * Filter a working table with '='-literals (equalities and inequalities)
 * over parameters and constants. `applied[k]` records that literal k has
 * been enforced (a literal can only be enforced once its columns appear in
 * the table, so this is called again after every join).
 */
void filter_equalities(const std::vector<Atom> &equalities,
                       Table &working_table,
                       std::vector<bool> &applied);

}  // namespace join_program

#endif  // DATABASE_JOIN_PROGRAM_H
