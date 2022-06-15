#ifndef SEARCH_DATALOG_TRANSFORMATIONS_CONNECTED_COMPONENTS_H_
#define SEARCH_DATALOG_TRANSFORMATIONS_CONNECTED_COMPONENTS_H_

class Graph {
    std::vector<int> nodes;
    std::vector<std::vector<int>> edges;

    std::vector<int> dfs(int i, std::vector<bool> &visited) {
        visited[i] = true;
        std::vector<int> ret;
        ret.push_back(i);
        for (int j : edges[i]) {
            if (!visited[j]) {
                std::vector<int> tmp = dfs(j, visited);
                ret.insert(ret.end(), tmp.begin(), tmp.end());
            }
        }
        std::sort(ret.begin(), ret.end());
        return ret;
    }

public:

    Graph(int max) {
        edges.resize(max);
        nodes.resize(0);
    }

    void add_node(int i) {
        nodes.push_back(i);
    }

    void add_edge(int i, int j) {
        edges[i].push_back(j);
    }

    std::vector<std::vector<int>> get_connected_components() {
        std::vector<std::vector<int>> components;
        std::vector<bool> visited(nodes.size(), false);
        for (size_t i = 0; i < nodes.size(); i++) {
            if (!visited[i]) {
                components.push_back(dfs(i, visited));
            }
        }
        return components;
    }

};

namespace datalog {

VariableSource update_source_after_component_split(std::unique_ptr<RuleBase> &original_rule,
                                                   const std::vector<int> &component,
                                                   int component_counter,
                                                   const VariableSource &source_new_split_rule) {// Update variable new_source of original rule
    VariableSource new_source = original_rule->get_variable_source_object();
    int counter = 0;
    for (auto entry : new_source.get_table()) {
        /*
         * TODO Change so it works for indirect entries as well. Currently, it only works for direct
         * queries, but it should work for both if we do a case splitting.
         */
        if (entry.first >= 0 or (std::find(component.begin(), component.end(), new_source.get_position_of_atom_in_same_body_rule(entry.first)) == component.end())) {
            counter++;
            continue;
        }

        int term = new_source.get_term_from_table_entry_index(counter);
        int new_position_in_condition = component_counter;
        int new_position_in_indirect_table = -1;

        int entry_position_indirect_table = source_new_split_rule.get_table_entry_index_from_term(term);
        new_position_in_indirect_table = entry_position_indirect_table ;

        new_source.update_ith_entry(counter, new_position_in_condition, new_position_in_indirect_table);
        counter++;
    }
    return new_source;
}

std::vector<std::vector<int>> get_components(std::unique_ptr<RuleBase> &rule) {

    std::vector<int> variables = rule->get_variables_in_body();

    std::vector<int> trivial_components;
    Graph g(rule->get_conditions().size());

    int condition_counter = 0;
    for (const auto &conditions : rule->get_conditions()) {
        if (conditions.is_nullary() or conditions.is_ground()) {
            g.add_node(condition_counter);
        } else {
            g.add_node(condition_counter);
            for (size_t j = condition_counter + 1; j < rule->get_conditions().size(); ++j) {
                if (rule->get_conditions()[condition_counter].share_variables(rule->get_conditions()[j])) {
                    g.add_edge(condition_counter, j);
                    g.add_edge(j, condition_counter);
                }
            }
        }
        ++condition_counter;
    }

    std::vector<std::vector<int>> components = g.get_connected_components();
    for (int c : trivial_components)
        components.push_back(std::vector<int>(1, c));
    return components;

}

DatalogAtom Datalog::split_connected_component(std::unique_ptr<RuleBase> &original_rule, const std::vector<int> &component, std::vector<std::unique_ptr<RuleBase>> &new_rules, int component_counter) {

    if (component.size() == 1) {
        // Update source table
        VariableSource new_source = original_rule->get_variable_source_object();
        int condition_in_component = component[0];
        int counter = 0;
        for (auto entry : new_source.get_table()) {
            if (new_source.get_position_of_atom_in_same_body_rule(entry.first) != condition_in_component) {
                counter++;
                continue;
            }
            if (entry.first >= 0)
                new_source.update_ith_entry(counter, component_counter, entry.second);
            else
                new_source.update_ith_entry(counter, (-1*component_counter)-1, entry.second);
            counter++;
        }
        original_rule->update_variable_source_table(std::move(new_source));
        return original_rule->get_conditions()[component[0]];
    }

    std::string predicate_name = "p$" + std::to_string(predicate_names.size());
    int idx = get_next_auxiliary_predicate_idx();
    map_new_predicates_to_idx.emplace(predicate_name, idx);
    predicate_names.push_back(predicate_name);

    std::vector<DatalogAtom> original_conditions = original_rule->get_conditions();
    std::vector<DatalogAtom> new_rule_conditions;
    new_rule_conditions.reserve(component.size());
    for (size_t id : component) {
        new_rule_conditions.push_back(original_conditions[id]);
    }

    Arguments new_args = get_relevant_joining_arguments_from_component(original_rule->get_effect(),
                                                                       new_rule_conditions);

    DatalogAtom new_atom(new_args, idx, true);
    std::unique_ptr<GenericRule> new_split_rule = std::make_unique<GenericRule>(0,
                                                                          new_atom,
                                                                          new_rule_conditions,
                                                                          nullptr);
    VariableSource new_source = update_source_after_component_split(original_rule,
                                                                    component,
                                                                    component_counter,
                                                                    new_split_rule->get_variable_source_object_by_ref());

    original_rule->update_variable_source_table(std::move(new_source));
    new_rules.push_back(std::move(new_split_rule));

    return new_atom;
}


void Datalog::split_into_connected_components(std::unique_ptr<RuleBase> &rule, std::vector<std::unique_ptr<RuleBase>> &new_rules) {
    std::vector<std::vector<int>> components = get_components(rule);

    if (components.size() == 1) return;

    std::map<int, int> map_condition_to_component;
    for (size_t i = 0; i < rule->get_conditions().size(); ++i) {
        for (size_t j = 0; j < components.size(); ++j) {
            if (std::find(components[j].begin(), components[j].end(), i) != components[j].end()) {
                map_condition_to_component[i] = j;
            }
        }
    }

    std::vector<DatalogAtom> original_conditions = rule->get_conditions();
    std::vector<DatalogAtom> new_rule_conditions;

    int component_counter = 0;
    for (const auto &component : components) {
        new_rule_conditions.push_back(split_connected_component(rule, component, new_rules, component_counter++));
    }

    rule->set_conditions(new_rule_conditions);

}

}

#endif //SEARCH_DATALOG_TRANSFORMATIONS_CONNECTED_COMPONENTS_H_
