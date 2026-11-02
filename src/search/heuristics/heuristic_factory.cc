#include "heuristic_factory.h"

#include "add_heuristic.h"
#include "blind_heuristic.h"
#include "ff_heuristic.h"
#include "goalcount.h"
#include "ground_delete_relaxation_heuristic.h"
#include "hmax_heuristic.h"
#include "rff_heuristic.h"

#include "datalog_transformation_options.h"

#include "../grounding/ground_task.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <optional>

#include "../utils/system.h"
#include "../utils/string_utils.h"

/*
  If the task was ground, the delete-relaxation heuristics are computed on
  the ground task (or on its decomposed relaxed program if it has too many
  ground operators) instead of the Datalog program.
*/
static Heuristic *create_ground_heuristic(const std::string &method, const Task &task)
{
    const std::shared_ptr<GroundTask> &ground_task = task.get_ground_task();
    std::optional<GroundDeleteRelaxationHeuristic::Type> type =
        GroundDeleteRelaxationHeuristic::get_type(method);
    if (!ground_task || !type)
        return nullptr;
    if (ground_task->relaxed_program_only)
        std::cout << "Using " << method << " heuristic on the decomposed relaxed program."
                  << std::endl;
    else
        std::cout << "Using ground " << method << " heuristic." << std::endl;
    return new GroundDeleteRelaxationHeuristic(task, ground_task, *type);
}

Heuristic *HeuristicFactory::create(const Options &opt, const Task &task)
{
    const std::string &method = opt.get_evaluator();

    std::cout << "Creating heuristic factory..." << std::endl;

    if (Heuristic *h = create_ground_heuristic(method, task))
        return h;

    if (utils::iequals(method, "blind")) {
        return new BlindHeuristic();
    }
    else if (utils::iequals(method, "add")) {
        return new AdditiveHeuristic(task, DatalogTransformationOptions());
    }
    else if (utils::iequals(method, "ff")) {
        return new FFHeuristic(task, DatalogTransformationOptions());
    }
    else if (utils::iequals(method, "goalcount")) {
        return new Goalcount();
    }
    else if (utils::iequals(method, "hmax")) {
        return new HMaxHeuristic(task, DatalogTransformationOptions());
    }
    else if (utils::iequals(method, "rff")) {
        return new RFFHeuristic(task, DatalogTransformationOptions());
    }
    else {
        std::cerr << "Invalid heuristic \"" << method << "\"" << std::endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }
}

Heuristic *HeuristicFactory::create_delete_free_heuristic(const std::string &method,
                                                          const Task &task)
{
    if (Heuristic *h = create_ground_heuristic(method, task))
        return h;
    if (utils::iequals(method, "add")) {
        return new AdditiveHeuristic(task);
    }
    else if (utils::iequals(method, "ff")) {
        return new FFHeuristic(task);
    }
    else if (utils::iequals(method, "hmax")) {
        return new HMaxHeuristic(task, DatalogTransformationOptions());
    }
    else if (utils::iequals(method, "rff")) {
        return new RFFHeuristic(task);
    }
    else {
        std::cerr << "Invalid delete-free heuristic \"" << method << "\"" << std::endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }
}
