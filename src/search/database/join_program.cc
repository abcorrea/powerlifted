#include "join_program.h"

#include <algorithm>
#include <cassert>

using namespace std;

namespace join_program {

void get_indices_and_constants(vector<int> &indices,
                               vector<int> &constants,
                               const Atom &a)
{
    int cont = 0;
    for (Argument arg : a.get_arguments()) {
        if (!arg.is_constant())
            indices.push_back(arg.get_index());
        else {
            indices.push_back((arg.get_index() + 1) * -1);
            constants.push_back(cont);
        }
        cont++;
    }
}

/*
 * Select only those tuples matching the constants of a partially grounded
 * atom.
 */
void select_tuples(const DBState &s,
                   const Atom &a,
                   vector<GroundAtom> &tuples,
                   const vector<int> &constants)
{
    for (const GroundAtom &atom : s.get_relations()[a.get_predicate_symbol_idx()].tuples) {
        bool match_constants = true;
        for (int c : constants) {
            assert(a.get_arguments()[c].is_constant());
            if (atom[c] != a.get_arguments()[c].get_index()) {
                match_constants = false;
                break;
            }
        }
        if (match_constants) tuples.push_back(atom);
    }
}

PrecompiledJoinProgram precompile(vector<Atom> &&atoms,
                                  const vector<bool> &is_static,
                                  const StaticInformation &static_information)
{
    PrecompiledJoinProgram program;
    program.relevant_atoms = std::move(atoms);

    // Create N empty tables
    program.precompiled_db.resize(program.relevant_atoms.size());

    for (size_t i = 0; i < program.relevant_atoms.size(); ++i) {
        const Atom &atom = program.relevant_atoms[i];

        if (!is_static[atom.get_predicate_symbol_idx()]) {
            // If the atom is fluent, we just flag it as such and we're done:
            // we'll have to deal with it during search time
            program.fluent_tables.push_back(i);
            continue;
        }

        // Otherwise the atom is static, so we precompile its table
        vector<GroundAtom> tuples;
        vector<int> constants, indices;

        get_indices_and_constants(indices, constants, atom);

        select_tuples(static_information, atom, tuples, constants);

        if (tuples.empty()) {
            program.statically_inapplicable = true;
            return program;
        }

        program.precompiled_db[i] = Table(std::move(tuples), std::move(indices));
    }

    return program;
}

bool fill_tables(const PrecompiledJoinProgram &program,
                 const DBState &state,
                 vector<Table> &tables)
{
    /*
     * Parse the state and the query atoms into a set of tables to perform
     * the join program more easily.
     *
     * We first obtain all indices in the atom that are constants. Then, we
     * create the table applying the projection over the arguments that
     * satisfy the instantiation of the constants. There are two cases for
     * the projection:
     *    1. The table comes from the static information (precompiled); or
     *    2. The table comes directly from the current state.
     */
    if (program.statically_inapplicable) return false;

    tables = program.precompiled_db;  // This performs the copy that we'll return
    for (unsigned i : program.fluent_tables) {
        // Let's fill in those (currently empty) tables that correspond to
        // fluent symbols in the query
        const Atom &atom = program.relevant_atoms[i];

        vector<GroundAtom> tuples;
        vector<int> constants, indices;

        get_indices_and_constants(indices, constants, atom);
        select_tuples(state, atom, tuples, constants);

        if (tuples.empty()) return false;

        tables[i] = Table(std::move(tuples), std::move(indices));
    }

    return true;
}

void filter_equalities(const vector<Atom> &equalities,
                       Table &working_table,
                       vector<bool> &applied)
{
    const auto &tup_idx = working_table.tuple_index;

    for (size_t k = 0; k < equalities.size(); ++k) {
        // Once a literal has been enforced, every later join only adds
        // columns and recombines surviving tuples, so the constrained columns
        // keep their (already valid) values — re-filtering is a guaranteed
        // no-op. Skip the ones already applied in an earlier join iteration.
        if (applied[k]) continue;
        const Atom &atom = equalities[k];
        const vector<Argument> &args = atom.get_arguments();
        assert(args.size() == 2);
        if (args[0].is_constant() && args[1].is_constant()) {
            bool is_equal = (args[0].get_index() == args[1].get_index());

            // Independent of the table columns, so its result is final.
            applied[k] = true;
            if ((atom.is_negated() && is_equal)
                    || (!atom.is_negated() && !is_equal)) {
                working_table.tuples.clear();
                return;
            }
        }
        else if (args[0].is_constant() || args[1].is_constant()) {
            int param_idx = -1;
            int const_idx = -1;
            if (args[0].is_constant()) {
                const_idx = args[0].get_index();
                param_idx = args[1].get_index();
            }
            else {
                const_idx = args[1].get_index();
                param_idx = args[0].get_index();
            }
            auto it = find(tup_idx.begin(), tup_idx.end(), param_idx);
            if (it != tup_idx.end()) {
                int index = distance(tup_idx.begin(), it);

                vector<Table::tuple_t> newtuples;
                for (const auto &t : working_table.tuples) {
                    if ((atom.is_negated() && t[index] != const_idx)
                            || (!atom.is_negated() && t[index] == const_idx)) {
                        newtuples.push_back(t);
                    }
                }
                working_table.tuples = std::move(newtuples);
                applied[k] = true;
            }
        }
        else { // !args[0].is_constant() && !args[1].is_constant()
            // TODO Revise this, looks that some work could be offloaded to
            //      preprocessing so that we do not need to do all this
            //      linear-time finds at runtime?
            auto it_1 = find(tup_idx.begin(), tup_idx.end(), args[0].get_index());
            auto it_2 = find(tup_idx.begin(), tup_idx.end(), args[1].get_index());

            if (it_1 != tup_idx.end() and it_2 != tup_idx.end()) {
                int index1 = distance(tup_idx.begin(), it_1);
                int index2 = distance(tup_idx.begin(), it_2);

                vector<Table::tuple_t> newtuples;
                for (const auto &t : working_table.tuples) {
                    if ((atom.is_negated() && t[index1] != t[index2])
                            || (!atom.is_negated() && t[index1] == t[index2])) {
                        newtuples.push_back(t);
                    }
                }
                working_table.tuples = std::move(newtuples);
                applied[k] = true;
            }
        }
    }
}

}  // namespace join_program
