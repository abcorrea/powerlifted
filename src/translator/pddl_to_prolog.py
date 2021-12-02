#! /usr/bin/env python3

import copy
import itertools
import sys

import normalize
import pddl
import timers

class PrologProgram:
    def __init__(self):
        self.facts = []
        self.rules = []
        self.objects = set()
        def predicate_name_generator():
            for count in itertools.count():
                yield "p$%d" % count
        self.new_name = predicate_name_generator()
    def add_fact(self, atom, weight=0):
        self.facts.append(Fact(atom, weight))
        self.objects |= set(atom.args)
    def add_rule(self, rule):
        self.rules.append(rule)
    def sort_facts(self):
        self.facts.sort(key=lambda x: str(x))
    def sort_rules(self):
        self.rules.sort(key=lambda x: str(x))
    def dump(self, file=None):
        for fact in self.facts:
            print(fact, file=file)
        for rule in self.rules:
            print(getattr(rule, "type", "none"), rule, file=file)
    def normalize(self):
        # Normalized prolog programs have the following properties:
        # 1. Each variable that occurs in the effect of a rule also occurs in its
        #    condition.
        # 2. The variables that appear in each effect or condition are distinct.
        # 3. There are no rules with empty condition.
        self.remove_free_effect_variables()
        self.split_duplicate_arguments()
        self.convert_trivial_rules()
    def split_rules(self):
        import split_rules

        # First, save original rules for further preprocessing
        self.sort_rules()
        self.original_rules = copy.deepcopy(self.rules)

        # Splits rules whose conditions can be partitioned in such a way that
        # the parts have disjoint variable sets, then split n-ary joins into
        # a number of binary joins, introducing new pseudo-predicates for the
        # intermediate values.
        new_rules = []
        for rule in self.rules:
            new_rules += split_rules.split_rule(rule, self.new_name)
        self.rules = new_rules
        self.sort_rules()
    def remove_free_effect_variables(self):
        """Remove free effect variables like the variable Y in the rule
        p(X, Y) :- q(X). This is done by introducing a new predicate
        @object, setting it true for all objects, and translating the above
        rule to p(X, Y) :- q(X), @object(Y).
        After calling this, no new objects should be introduced!"""

        # Note: This should never be necessary for typed domains.
        # Leaving it in at the moment regardless.
        must_add_predicate = False
        for rule in self.rules:
            eff_vars = get_variables([rule.effect])
            cond_vars = get_variables(rule.conditions)
            if not eff_vars.issubset(cond_vars):
                must_add_predicate = True
                eff_vars -= cond_vars
                for var in sorted(eff_vars):
                    rule.add_condition(pddl.Atom("@object", [var]))
        if must_add_predicate:
            print("Unbound effect variables: Adding @object predicate.")
            self.facts += [Fact(pddl.Atom("@object", [obj])) for obj in self.objects]
    def split_duplicate_arguments(self):
        """Make sure that no variable occurs twice within the same symbolic fact,
        like the variable X does in p(X, Y, X). This is done by renaming the second
        and following occurrences of the variable and adding equality conditions.
        For example p(X, Y, X) is translated to p(X, Y, X@0) with the additional
        condition =(X, X@0); the equality predicate must be appropriately instantiated
        somewhere else."""
        printed_message = False
        for rule in self.rules:
            if rule.rename_duplicate_variables() and not printed_message:
                print("Duplicate arguments: Adding equality conditions.")
                printed_message = True

    def convert_trivial_rules(self):
        """Convert rules with an empty condition into facts.
        This must be called after bounding rule effects, so that rules with an
        empty condition must necessarily have a variable-free effect.
        Variable-free effects are the only ones for which a distinction between
        ground and symbolic atoms is not necessary."""
        must_delete_rules = []
        for i, rule in enumerate(self.rules):
            if not rule.conditions:
                assert not get_variables([rule.effect])
                self.add_fact(pddl.Atom(rule.effect.predicate, rule.effect.args), rule.weight)
                must_delete_rules.append(i)
        if must_delete_rules:
            print("Trivial rules: Converted to facts.")
            for rule_no in must_delete_rules[::-1]:
                del self.rules[rule_no]

    def remove_action_predicates(self):
        '''
        Remove the action predicates and restructure the Datalog program.
        For example,

        join action_a(?x, ?b) :- p(?x, ?b), r(?x).
        project eff_1(?x) :- action_a(?x, ?b).
        project eff_2(?b) :- action_a(?x, ?b).

        becomes

        join eff_1(?x) :- p(?x, ?b), r(?x).
        join eff_2(?x) :- p(?x, ?b), r(?x).

        This *needs* to be made before the renaming.
        '''

        non_action_rules = []
        action_rules = dict()
        for r in self.rules:
            # Capture action rules and do not add them to the new set of rules
            rule_name = str(r.effect)
            if rule_name.startswith("action_"):
                action_rules[rule_name] = r
            else:
                non_action_rules.append(r)

        final_rules = []
        for r in non_action_rules:
            if len(r.conditions) == 1:
                condition_name = str(r.conditions[0])
                if condition_name in action_rules.keys():
                    new_action_rule = copy.deepcopy(action_rules[condition_name])
                    new_action_rule.effect = r.effect
                    # TODO If we use lifted costs, this should be done before
                    new_action_rule.weight = action_rules[condition_name].weight
                    final_rules.append(new_action_rule)
                else:
                    # TODO If we use lifted costs, this should be done before
                    r.weight = 0
                    final_rules.append(r)
            else:
                final_rules.append(r)
        self.rules = final_rules
        self.sort_rules()

    def rename_free_variables(self):
        '''
        Use canonical names for free variables. The names are based on the
        order in
        which the variables first show up and not on the PDDL file.
        '''


        def is_free_var(var, num):
            if var[0] != '?':
                #new_effect.append(var)
                return False, 0
            if var not in parameter_to_generic_free_var.keys():
                parameter_to_generic_free_var[var] = "?var" + str(num)
                return True, 1
            else:
                return True, 0

        new_rules = []
        for r in self.rules:
            rule = copy.deepcopy(r)
            parameter_to_generic_free_var = dict()
            num_free_vars = 0
            new_effect = []
            for index, e in enumerate(rule.effect.args):
                is_free, increase = is_free_var(e, num_free_vars)
                if is_free:
                    new_effect.append(parameter_to_generic_free_var[e])
                    num_free_vars += increase
                else:
                    new_effect.append(e)
            rule.effect.args = tuple(new_effect)
            for index, c in enumerate(rule.conditions):
                new_condition = []
                for a in c.args:
                    is_free, increase = is_free_var(a, num_free_vars)
                    if is_free:
                        new_condition.append(parameter_to_generic_free_var[a])
                        num_free_vars += increase
                    else:
                        new_condition.append(a)
                rule.conditions[index].args = tuple(new_condition)
            new_rules.append(rule)
        self.rules = new_rules

    def find_equivalent_rules(self, rules):
        has_duplication = False
        new_rules = []
        remaining_equivalent_rules = dict()
        equivalence = dict()
        for rule in rules:
            if "p$" in str(rule.effect):
                '''Auxiliary variable'''
                if (str(rule.conditions), str(rule.effect.args)) in remaining_equivalent_rules.keys():
                    equivalence[str(rule.effect.predicate)] = remaining_equivalent_rules[(str(rule.conditions), str(rule.effect.args))]
                    has_duplication = True
                    continue
                remaining_equivalent_rules[(str(rule.conditions), str(rule.effect.args))] = rule.effect.predicate
            new_rules.append(rule)
        return has_duplication, new_rules, equivalence

    def remove_duplicated_rules(self):
        '''
        Remove redundant and duplicated rules from the IDB of the Datalog
        '''
        has_duplication = True
        total_rules_removed = 0
        while has_duplication:
            number_removed = 0
            final_rules = []
            has_duplication, new_rules, equivalence = self.find_equivalent_rules(self.rules)
            for rule in new_rules:
                for i, c in enumerate(rule.conditions):
                    pred_symb = str(c.predicate)
                    if pred_symb in equivalence.keys():
                        new_cond = c
                        new_cond.predicate = equivalence[pred_symb]
                        number_removed += 1
                        #print("Replace %s by %s" % (pred_symb, equivalence[pred_symb]))
                        rule.conditions[i] = new_cond
                final_rules.append(rule)
            total_rules_removed += number_removed
            self.rules = final_rules
        #print("Total number of duplicated rules removed: %d" % total_rules_removed, file=sys.stderr)

    def remove_fluent_atoms_from_edb(self, task):
        fluents = set()
        new_facts = []
        for a in task.actions:
            for e in a.effects:
                l = e.literal
                pred = l.predicate
                fluents.add(pred)
        for fact in self.facts:
            if fact.atom.predicate not in fluents:
                new_facts.append(fact)
        self.facts = new_facts
        self.sort_facts()


def get_variables(symbolic_atoms):
    variables = set()
    for sym_atom in symbolic_atoms:
        variables |= {arg for arg in sym_atom.args if arg[0] == "?"}
    return variables

class Fact:
    def __init__(self, atom, weight=0):
        self.atom = atom
        self.weight = weight
    def __str__(self):
        return "%s [%s]." % (self.atom, str(self.weight))

class Rule:
    def __init__(self, conditions, effect, weight=0):
        self.conditions = conditions
        self.effect = effect
        self.weight = weight
    def add_condition(self, condition):
        self.conditions.append(condition)
    def get_variables(self):
        return get_variables(self.conditions + [self.effect])
    def _rename_duplicate_variables(self, atom, new_conditions):
        used_variables = set()
        for i, var_name in enumerate(atom.args):
            if var_name[0] == "?":
                if var_name in used_variables:
                    new_var_name = "%s@%d" % (var_name, len(new_conditions))
                    atom = atom.replace_argument(i, new_var_name)
                    new_conditions.append(pddl.Atom("=", [var_name, new_var_name]))
                else:
                    used_variables.add(var_name)
        return atom
    def rename_duplicate_variables(self):
        extra_conditions = []
        self.effect = self._rename_duplicate_variables(
            self.effect, extra_conditions)
        old_conditions = self.conditions
        self.conditions = []
        for condition in old_conditions:
            self.conditions.append(self._rename_duplicate_variables(
                    condition, extra_conditions))
        self.conditions += extra_conditions
        return bool(extra_conditions)

    def __str__(self):
        cond_str = ", ".join(map(str, self.conditions))
        return "%s :- %s [%s]." % (self.effect, cond_str, self.weight)


def translate_typed_object(prog, obj, type_dict):
    supertypes = type_dict[obj.type_name].supertype_names
    for type_name in [obj.type_name] + supertypes:
        prog.add_fact(pddl.TypedObject(obj.name, type_name).get_atom())

def translate_facts(prog, task):
    type_dict = {type.name: type for type in task.types}
    for obj in task.objects:
        translate_typed_object(prog, obj, type_dict)
    for fact in task.init:
        assert isinstance(fact, pddl.Atom) or isinstance(fact, pddl.Assign)
        if isinstance(fact, pddl.Atom):
            prog.add_fact(fact)
    # Sort facts to preserve detemrinistic ordering over multiple runs
    prog.sort_facts()

def add_inequalities(prog,task):
    for obj1 in task.objects:
        for obj2 in task.objects:
            if obj1 == obj2:
                continue
            a = pddl.Atom(normalize.NOT_EQUAL_PREDICATE, [obj1.name, obj2.name])
            prog.add_fact(a, 0)


def get_action_cost(action):
    if action is None:
        # Rule precondition is an effect condition, axiom, or goal
        return 0
    cost = action.cost
    if action.cost is None:
        cost = 0
    if isinstance(action.cost, pddl.Increase):
        if isinstance(action.cost.expression, pddl.NumericConstant):
            cost = action.cost.expression.value
        else:
            cost = 1
    return cost

def translate(task, keep_action_predicates=False, add_inequalities_flag=False):
    # Note: The function requires that the task has been normalized.
    prog = PrologProgram()
    translate_facts(prog, task)
    if add_inequalities_flag:
        add_inequalities(prog, task)
    for conditions, effect, action in normalize.build_exploration_rules(task, add_inequalities_flag):
        weight = get_action_cost(action)
        # We sort the conditions to make the output deterministic.
        # They are sorted by reversed alphabetical order. This shows better results than
        # alphabetical order. Our hypothesis is that this ordering puts type@ predicates
        # at the beginning of the condition and thus they are split earlier. Since type predicates
        # are all unary, this works as a semi-join, filtering intermediate relations.
        conditions.sort(key=lambda x: str(x))
        conditions.reverse()
        if effect.predicate == "@goal-reachable":
            weight = 0
        prog.add_rule(Rule(conditions, effect, weight))
    prog.sort_rules()
    # Using block=True because normalization can output some messages
    # in rare cases.
    prog.remove_fluent_atoms_from_edb(task)
    if not keep_action_predicates:
        prog.remove_action_predicates()
    prog.normalize()
    prog.split_rules()
    return prog



if __name__ == "__main__":
    import pddl_parser
    task = pddl_parser.open()
    normalize.normalize(task)
    prog = translate(task)
    #prog.rename_free_variables()
    #prog.remove_duplicated_rules()
    prog.dump()
