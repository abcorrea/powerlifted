#ifndef SEARCH_HEURISTICS_DATALOG_TRANSFORMATION_OPTS_H
#define SEARCH_HEURISTICS_DATALOG_TRANSFORMATION_OPTS_H


class DatalogTransformationOptions {
    bool rename_vars;
    bool collapse_predicates;
    bool remove_action_predicates;

public:
    DatalogTransformationOptions() : rename_vars(true), collapse_predicates(true), remove_action_predicates(true)
    {}

    DatalogTransformationOptions(bool rename_vars,
                                 bool collapse_predicates,
                                 bool remove_action_predicates) : rename_vars(rename_vars),
                                                                  collapse_predicates(collapse_predicates),
                                                                  remove_action_predicates(remove_action_predicates)
    {}

    bool get_rename_vars() const {return rename_vars;}

    bool get_collapse_predicates() const {return collapse_predicates;}

    bool get_remove_action_predicates() const {return remove_action_predicates;}

};

#endif