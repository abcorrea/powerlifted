#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include "task.h"
#include "assert.h"


constexpr const char* domainName = "tmp-domain";

std::string max_pattern(size_t var_amount) {
    std::stringstream str;
    str << "pdb(pattern=manual_pattern(pattern=[";
    for (size_t i = 0; i < var_amount; i++) {
        str << i;
        if (i < var_amount-1) {
            str << ",";
        }
    }
    str << "]))";
    return str.str();
}


void dumpToPDDLDomain(const Task& task, const std::string& domainFileName) {
    printf("dump to pddl is wk");
    std::ofstream domainFile(domainFileName);
    assert(domainFile.is_open());

    domainFile << "(define (domain " << domainName << ")\n";
    domainFile << "  (:requirements :strips)\n\n";

    domainFile << "  ;; Define predicates\n";
    domainFile << "  (:predicates\n";
    for (const Predicate& predicate : task.predicates) {
        domainFile << "    (" << predicate.get_name();
        for (int i = 0; i < predicate.getArity(); ++i) {
            domainFile << " ?arg" << i;
        }
        domainFile << ")\n";
    }
    // domainFile << "    (" << startPred << ")\n";
    domainFile << "  )\n";

    domainFile << "  ;; Define actions\n";
    for (const ActionSchema& actionSchema : task.get_action_schemas()) {
        domainFile << "  (:action " << actionSchema.get_name() << "\n";
        domainFile << "    :parameters (";
        for (int i = 0; i < actionSchema.get_parameters().size(); ++i) {
            domainFile << actionSchema.get_parameters().at(i).name;
            if (i < actionSchema.get_parameters().size() - 1) {
                domainFile << " ";
            }
        }
        domainFile << ")\n";

        domainFile << "    :precondition (and ";
        for (const Atom& precondition : actionSchema.get_precondition()) {
            domainFile << "(";
            if (precondition.is_negated()) {
                domainFile << "not (";
            }
            domainFile << task.predicates.at(precondition.get_predicate_symbol_idx()).get_name();
            for (auto &arg : precondition.get_arguments()) {
                if (arg.is_constant()) {
                    domainFile << " " << task.get_object_name(arg.get_index());
                } else {
                    domainFile << " " << actionSchema.get_parameters().at(arg.get_index()).name;
                }
            }
            if (precondition.is_negated()) {
                domainFile << ")";
            }
            domainFile << ") ";
        }
        for (size_t i = 0; i < actionSchema.get_positive_nullary_precond().size(); i++) {
            if (actionSchema.get_positive_nullary_precond().at(i)) {
                domainFile << "(" << task.get_predicate_name(i) << ") ";
            }
        }
        for (size_t i = 0; i < actionSchema.get_negative_nullary_precond().size(); i++) {
            if (actionSchema.get_negative_nullary_precond().at(i)) {
                domainFile << "(not(" << task.get_predicate_name(i) << ")) ";
            }
        }
        domainFile << ")\n";

        domainFile << "    :effect (and ";
        for (const Atom& effect : actionSchema.get_effects()) {
            domainFile << "(";
            if (effect.is_negated()) {
                domainFile << "not (";
            }
            domainFile << task.predicates.at(effect.get_predicate_symbol_idx()).get_name();
            for (auto &arg : effect.get_arguments()) {
                if (arg.is_constant()) {
                    domainFile << " " << task.get_object_name(arg.get_index());
                } else {
                    domainFile << " " << actionSchema.get_parameters().at(arg.get_index()).name;
                }
            }
            if (effect.is_negated()) {
                domainFile << ")";
            }
            domainFile << ") ";
        }
        for (size_t i = 0; i < actionSchema.get_positive_nullary_effects().size(); i++) {
            if (actionSchema.get_positive_nullary_effects().at(i)) {
                domainFile << "(" << task.get_predicate_name(i) << ") ";
            }
        }
        for (size_t i = 0; i < actionSchema.get_negative_nullary_effects().size(); i++) {
            if (actionSchema.get_negative_nullary_effects().at(i)) {
                domainFile << "(not(" << task.get_predicate_name(i) << ")) ";
            }
        }
        domainFile << ")\n  )\n";
    }


    // Close the domain file
    domainFile << ")\n";
    domainFile.close();

    std::cout << "Domain file '" << domainFileName << "' created successfully." << std::endl;
}

void dumpToPDDLProblem(const DBState& initialState, const Task& task, const std::string& problemFileName) {
    std::ofstream problemFile(problemFileName);
    assert(problemFile.is_open());

    problemFile << "(define (problem lifted-planning)\n";
    problemFile << "  (:domain " << domainName << ")\n\n";

    problemFile << "  ;; Define objects\n";
    problemFile << "  (:objects";
    for (auto &obj : task.objects) {
        problemFile << " " << obj.get_name();
    }
    problemFile << ")\n\n";

    problemFile << "  ;; Define initial state\n";
    problemFile << "  (:init \n";
    for (const auto& relation : initialState.get_relations()) {
        for (const auto& tuple : relation.tuples) {
            problemFile << "    (" << task.get_predicate_name(relation.predicate_symbol);
            for (auto i : tuple) {
                problemFile << " " << task.get_object_name(i);
            }
            problemFile << ")\n";
        }
    }
    for (size_t i = 0; i < initialState.get_nullary_atoms().size(); i++) {
        if (initialState.get_nullary_atoms().at(i)) {
            problemFile << "    (" << task.get_predicate_name(i) << ") ";
        }
    }
    // problemFile << "    (" << startPred << ")\n";
    problemFile << "  )\n\n";


    problemFile << "  ;; Define goal\n";
    problemFile << "  (:goal \n";
    problemFile << "    (and \n";
    for (const auto& relation : task.get_goal().goal) {
        problemFile << "      (";
        if (relation.is_negated()) {
            problemFile << "not (";
        }
        problemFile << task.get_predicate_name(relation.get_predicate_index());
        for (const auto i : relation.get_arguments()) {
            problemFile << " " << task.get_object_name(i);
        }
        if (relation.is_negated()) {
            problemFile << ")";
        }
        problemFile << ")\n";
    }
    for (auto i : task.get_goal().positive_nullary_goals) {
        problemFile << "(" << task.get_predicate_name(i) << ") ";
    }
    for (auto i : task.get_goal().negative_nullary_goals) {
        problemFile << "(not (" << task.get_predicate_name(i) << ")) ";
    }
    problemFile << "    )\n";
    problemFile << "  )\n";

    // Close the problem file
    problemFile << ")\n";
    problemFile.close();

    std::cout << "Problem file '" << problemFileName << "' created successfully.\n";
}
