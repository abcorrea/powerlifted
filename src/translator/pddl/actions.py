from __future__ import print_function

import copy

from . import effects
from . import conditions


class Action(object):
    def __init__(self, name, parameters, num_external_parameters,
                 precondition, effects, cost):
        assert 0 <= num_external_parameters <= len(parameters)
        self.name = name
        self.parameters = parameters
        # num_external_parameters denotes how many of the parameters
        # are "external", i.e., should be part of the grounded action
        # name. Usually all parameters are external, but "invisible"
        # parameters can be created when compiling away existential
        # quantifiers in conditions.
        self.num_external_parameters = num_external_parameters
        self.precondition = precondition
        self.effects = effects
        self.cost = cost
        self.uniquify_variables()  # TODO: uniquify variables in cost?

    def __repr__(self):
        return "action_%s" % (self.name)

    def dump(self):
        print("%s(%s)" % (self.name, ", ".join(map(str, self.parameters))))
        print("Precondition:")
        self.precondition.dump()
        print("Effects:")
        for eff in self.effects:
            eff.dump()
        print("Cost:")
        if (self.cost):
            self.cost.dump()
        else:
            print("  None")

    def uniquify_variables(self):
        self.type_map = dict([(par.name, par.type_name)
                              for par in self.parameters])
        self.precondition = self.precondition.uniquify_variables(self.type_map)
        for effect in self.effects:
            effect.uniquify_variables(self.type_map)

    def relaxed(self):
        new_effects = []
        for eff in self.effects:
            relaxed_eff = eff.relaxed()
            if relaxed_eff:
                new_effects.append(relaxed_eff)
        return Action(self.name, self.parameters, self.num_external_parameters,
                      self.precondition.relaxed().simplified(),
                      new_effects)

    def untyped(self):
        # We do not actually remove the types from the parameter lists,
        # just additionally incorporate them into the conditions.
        # Maybe not very nice.
        result = copy.copy(self)
        parameter_atoms = [par.to_untyped_strips() for par in self.parameters]
        new_precondition = self.precondition.untyped()
        result.precondition = conditions.Conjunction(
            parameter_atoms + [new_precondition])
        result.effects = [eff.untyped() for eff in self.effects]
        return result

    @property
    def get_action_preconditions(self):
        """
        Return a list of preconditions of an action

        :return: list of preconditions
        """
        precond = []
        if isinstance(self.precondition, conditions.Atom):
            precond = [self.precondition]
        else:
            precond = list(self.precondition.parts)
        try:
            assert isinstance(precond, list)
        except AssertionError:
            raise NotImplementedError(
                'Preconditions must be a single atom or a conjunction of '
                'atoms.')
        return precond

    @property
    def get_literals_in_effects(self):
        """
        Return set with all literals occurring in effects, but not in effect
        conditions.

        :return: set of literals
        """
        literals = set()
        for eff in self.effects:
            try:
                assert isinstance(eff, effects.Effect)
            except AssertionError:
                raise NotImplementedError(
                    "Conditional effects are not supported.")
            for arg in eff.literal.args:
                literals.add(arg)
        return literals

    def transform_precondition_into_list(self):
        """
        Transform atomic precondition into list

        :return: void
        """
        if isinstance(self.precondition, conditions.Literal):
            self.precondition = conditions.Conjunction([self.precondition])
        return
