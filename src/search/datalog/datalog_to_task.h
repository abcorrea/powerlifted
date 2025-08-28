#ifndef SEARCH_DATALOG_DATALOG_TO_TASK_H_
#define SEARCH_DATALOG_DATALOG_TO_TASK_H_
#include "../task.h"
#include "datalog.h"
#include "datalog_atom.h"
#include <iostream>
#include "term.h"
class Datalog_to_task
{
const datalog::Datalog& datalog; 
Task task;
std::unordered_set<int> fluent_predicates;
std::unordered_set<int> static_predicates;

public:
Datalog_to_task(const datalog::Datalog&datalog): datalog(datalog),task("dl_domain","dl_task"){
}

const Task& getTask(){
    
    extract_predicates();
    // classify_predicates();
    // extract_objects();
    // set_type_names();
    // construct_initial_state();
    // convert_action_schema();
    // set_goal_state();

   return task;
}

private:
void extract_predicates(){
    int index=0;
    std::unordered_set<std::string> unique_predicate;

    // extract predicate from facts
    for (auto f : datalog.get_facts()){
        extract_predicate_from_datalogatom(f,unique_predicate,index);
    }
    // extract predicate from rules
    for (const auto &r : datalog.get_rules()){
        extract_predicate_from_datalogatom(r->get_effect(),unique_predicate,index);
        for(auto atom :r->get_conditions()){
            extract_predicate_from_datalogatom(atom,unique_predicate,index);
        }
    }

    //test
    
    std::cout << "=== DATALOG PREDICATE ANALYSIS ===" << std::endl;
    std::cout << "Total extracted predicates: " << task.predicates.size() << std::endl;
    
 
    try {
        int count = 0;
        for (int i = 0; i < 10000; ++i) {  
            try {
                std::string name = datalog.get_predicate_name_by_idx(i);
                count++;
                if (i < 20) {  
                    std::cout << "Datalog[" << i << "] = '" << name << "'" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "Datalog predicate_names size appears to be: " << count << std::endl;
                break;
            }
        }
    } catch (...) {
        std::cout << "Error while checking datalog predicate size" << std::endl;
    }
    
    std::cout << "=== END ANALYSIS ===" << std::endl;

   
};
void extract_predicate_from_datalogatom(const datalog::DatalogAtom & atom,std::unordered_set<std::string> & unique_predicate, int & index){
    int dl_index = atom.get_predicate_index();
    std::string name = datalog.get_predicate_name_by_idx(dl_index);
    if(unique_predicate.count(name)>0){
        return;
    }
    unique_predicate.insert(name);

    int arity= atom.get_arguments().size();
    std::vector<int> types(arity,0);
    int argu_pos =0;
    for(auto argu: atom.get_arguments()){
        if (argu.is_object()==false){
            types[argu_pos]=1;
        }else{
            types[argu_pos]=0;
        }
        argu_pos++;

    }
    
    
    
    task.add_predicate( name, index, arity,false,types);
    if (arity==0){
        task.nullary_predicates.insert(index);
    }
   
    index++;
        

}

void classify_predicates(){
    for (const auto &r : datalog.get_rules()){
        int index_dl = r->get_effect().get_predicate_index();
        int index_new_task= task.get_predicate_index(datalog.get_predicate_name_by_idx(index_dl));
        if(index_new_task!= -1){
            fluent_predicates.emplace(index_new_task);

        }else{std::cerr << "Error: can't find " << index_dl << " in new task predicates" << std::endl;}
    }
    for(auto p : task.predicates){

        int index_task = p.get_index();
        if(fluent_predicates.count(index_task)==0){
            static_predicates.emplace(index_task);
        }}
}
void extract_objects(){
    // for (auto o : datalog.get_objects()){
    //     task.add_object(o.get_name(),o.get_index(),o.get_types());
    // }
    for (size_t i=0;i< datalog.get_objects().size();i++){
        task.add_object(datalog.get_objects()[i].get_name(),datalog.get_objects()[i].get_index(),datalog.get_objects()[i].get_types());

    }
};
void set_type_names(){
    //no type specified in dump to pddl, 
    task.add_type("OBJECT");
    task.add_type("VARIABLE ");
    

}


void construct_initial_state(){
    task.create_empty_initial_state(task.predicates.size(),task.objects.size());
    for (auto atom:datalog.get_facts()){
       int dl_index= atom.get_predicate_index();
       std::string pred_name=datalog.get_predicate_name_by_idx(dl_index);
       int new_task_idx= task.get_predicate_index(pred_name);
       if (fluent_predicates.count(new_task_idx)>0){
        // a fluent predicate 

        if(task.nullary_predicates.count(new_task_idx)>0){
            // a nullary predicate
            task.initial_state.set_nullary_atom(new_task_idx, true);

        }else{
                // non-nullary
        auto a = atom.get_arguments();
        GroundAtom ga;
        for(const datalog::Term &t : a){
            ga.emplace_back(t.get_index());

        }
        task.initial_state.add_tuple(new_task_idx,ga);
        

        }

       }
       else{
        // a static predicate
           if(task.nullary_predicates.count(new_task_idx)>0){
            // a nullary predicate
            task.static_info.set_nullary_atom(new_task_idx,true);


        }
        //// non-nullary
        else{
                            // non-nullary
            auto a = atom.get_arguments();
            GroundAtom ga;
            for(const datalog::Term &t : a){
                ga.emplace_back(t.get_index());
            }
            task.static_info.add_tuple(new_task_idx,ga);
        

        }
       }
    }
    

}
void convert_action_schema(){
    std::vector<ActionSchema> actions;
    int action_schema_idx=0;
    
    for(const auto &r : datalog.get_rules()){
        if(r->get_effect().get_index()==(datalog.get_goal_atom_idx())){
            continue;
            
        }

        std::vector<Atom> pres;
        std::vector<Atom> effs;
        std::vector<Parameter> parameters;
        std::unordered_set<int>unique_param;
        std::unordered_map<int,int> var_org_new_idx;
    
        std::vector<bool> positive_nullary_precon(task.predicates.size(), false);
        std::vector<bool> positive_nullary_eff(task.predicates.size(), false);


        int par_index=0;
        for (auto pre : r->get_conditions()){
            int pre_idx=pre.get_index();
            std::string pred_name = datalog.get_predicate_name_by_idx(pre_idx);
            int new_idx= task.get_predicate_index(datalog.get_predicate_name_by_idx(pre_idx));
            if(new_idx == -1) {
                // std::cout << "Missing predicate: '" << pred_name << "' (datalog_index: " << pre_idx << ")" << std::endl;
                continue;
            }
            if(pre.is_nullary()){    
                if(new_idx >= 0 && new_idx < positive_nullary_precon.size()) {
                    positive_nullary_precon[new_idx]=true;
                 } 
                // else {
                //     std::cout << "Warning: new_idx " << new_idx << " out of bounds, size=" << positive_nullary_precon.size() << std::endl;
                //  }
                continue;
            }
            std::vector<Argument> arg_vec;
            for (auto arg: pre.get_arguments()){
                
                if(arg.is_object()!=1){
                    if(unique_param.count(arg.get_index())==0){
                        std::string para_name="var"+std::to_string(arg.get_index());
                        Parameter argument = Parameter(para_name,par_index,1);
                        parameters.emplace_back(argument);
                        var_org_new_idx[arg.get_index()]=par_index;
                        unique_param.emplace(arg.get_index());
                        par_index++;
                    }

                arg_vec.emplace_back(Argument(var_org_new_idx[arg.get_index()],false,false));
                }else{
                    arg_vec.emplace_back(Argument(arg.get_index(),true,false));
                }
                
            }
            int predicate_idx=pre.get_index();

            std::string name= datalog.get_predicate_name_by_idx(predicate_idx);
            int new_index= task.get_predicate_index(name);
        
            Atom pre_con = Atom(std::move(arg_vec),std::move(name),new_index,false);
            pres.emplace_back(pre_con);


        }// error detected above
        // auto eff = r->get_effect();
        // int eff_idx=eff.get_index();
        // if(eff.is_nullary()){
        //     positive_nullary_eff[eff_idx]=true;
        // }else{
        // std::vector<Argument> eff_vec;
        // for (auto arg: eff.get_arguments()){
            
        //     if(arg.is_object()!=1){
        //         if(unique_param.count(arg.get_index())==0){
        //             std::string para_name="var"+std::to_string(arg.get_index());
                    
        //             Parameter argument = Parameter(para_name,par_index,1);
        //             parameters.emplace_back(argument);
                    
        //             var_org_new_idx[arg.get_index()]=par_index;
        //             unique_param.emplace(arg.get_index());
        //             par_index++;
                
        //         }    
        //         eff_vec.emplace_back(Argument(var_org_new_idx[arg.get_index()],false,false));
        //     }else{
        //         eff_vec.emplace_back(Argument(arg.get_index(),true,false));

        //     }
                

        // }
        
        
        // std::string name= datalog.get_predicate_name_by_idx(eff_idx);
        // Atom eff_atom =  Atom(std::move(eff_vec),std::move(name),eff_idx,false);
        // effs.emplace_back(eff_atom);}
        

        auto cost = r->get_weight();
        std::string name="action_"+std::to_string(action_schema_idx);

        actions.emplace_back(ActionSchema(name,action_schema_idx,cost,parameters,{},pres,effs,{},positive_nullary_precon,{},positive_nullary_eff,{})) ;
        action_schema_idx++;

    }
    task.initialize_action_schemas(actions);
    // argument index problem fixed with help of claude.
}
void set_goal_state(){

    std::vector<AtomicGoal> goal;
    std::unordered_set<int> positive_nullary_goals;
    std::unordered_set<int> negative_nullary_goals;
     for(const auto &r : datalog.get_rules()){
        if(r->get_effect().get_index()==(datalog.get_goal_atom_idx())){
          for (auto goal_condition: r->get_conditions()){
            auto idx_atomic= goal_condition.get_index();
            std::vector<int> args;
            for (auto t :goal_condition.get_arguments()){
                args.emplace_back(t.get_index());
            }
            bool neg= false;
            
            if(goal_condition.is_nullary()){
                positive_nullary_goals.emplace(idx_atomic);
            }else{goal.emplace_back(AtomicGoal(idx_atomic,args,neg));}


          }
          task.create_goal_condition(goal,positive_nullary_goals,negative_nullary_goals);
          return;
        }
        
     }

    
    
}

};







#endif
