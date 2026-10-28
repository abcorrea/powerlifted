#include "heuristic_factory.h"

#include "add_heuristic.h"
#include "blind_heuristic.h"
#include "ff_heuristic.h"
#include "goalcount.h"
#include "hmax_heuristic.h"
#include "rff_heuristic.h"

#include "datalog_transformation_options.h"
#include "grounded_heuristic.h"

#include "../grounding/ground_task.h"
#include "../utils/timer.h"

#include <cstdlib>
#include <memory>

#include <fstream>
#include <iostream>

#include "../utils/system.h"
#include "../utils/string_utils.h"

static Heuristic *create_grounded(const std::string &method, const Task &task)
{
    GroundedHeuristic::Type type;
    if (utils::iequals(method, "add"))
        type = GroundedHeuristic::ADD;
    else if (utils::iequals(method, "ff"))
        type = GroundedHeuristic::FF;
    else if (utils::iequals(method, "hmax"))
        type = GroundedHeuristic::MAX;
    else
        return nullptr;
    std::shared_ptr<GroundTask> ground_task = GroundTask::get_cached(task);
    if (!ground_task) {
        // Too many ground operators: the relaxed program of the ground facts.
        ground_task = GroundTask::get_cached_relaxation(task);
        if (!ground_task)
            return nullptr;
        std::cout << "Using " << method << " heuristic on the decomposed relaxed program." << std::endl;
        return new GroundedHeuristic(task, ground_task, type);
    }
    std::cout << "Using grounded " << method << " heuristic." << std::endl;
    return new GroundedHeuristic(task, ground_task, type);
}

Heuristic *HeuristicFactory::create(const Options &opt, const Task &task)
{
    const std::string &method = opt.get_evaluator();

    std::cout << "Creating heuristic factory..." << std::endl;

    if (Heuristic *h = create_grounded(method, task))
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
    if (Heuristic *h = create_grounded(method, task))
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
