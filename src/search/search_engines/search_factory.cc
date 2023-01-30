
#include "search_factory.h"

#include "astar_search.h"
#include "alternated_bfws.h"
#include "breadth_first_search.h"
#include "breadth_first_width_search.h"
#include "dual_queue_bfws.h"
#include "greedy_best_first_search.h"
#include "lazy_search.h"
#include "search.h"

#include "../states/extensional_states.h"
#include "../states/sparse_states.h"

#include <boost/algorithm/string.hpp>

SearchBase*
SearchFactory::create(const Options &opt, const std::string& method, const std::string& state_type) {
    std::cout << "Creating search factory for method " << method << "..." << std::endl;
    bool using_ext_state = boost::iequals(state_type, "extensional");

    if (boost::iequals(method, "astar")) {
        if (using_ext_state) return new AStarSearch<ExtensionalPackedState>();
        else return new AStarSearch<SparsePackedState>();
    }
    else if (boost::iequals(method, "bfs")) {
        if (using_ext_state) return new BreadthFirstSearch<ExtensionalPackedState>();
        else return new BreadthFirstSearch<SparsePackedState>();
    }
    else if (boost::iequals(method, "bfws1")) {
        if (using_ext_state) return new BreadthFirstWidthSearch<ExtensionalPackedState>(1, opt, StandardNovelty::R_0);
        else return new BreadthFirstWidthSearch<SparsePackedState>(1, opt, StandardNovelty::R_0);
    }
    else if (boost::iequals(method, "bfws2")) {
        if (using_ext_state) return new BreadthFirstWidthSearch<ExtensionalPackedState>(2, opt, StandardNovelty::R_0);
        else return new BreadthFirstWidthSearch<SparsePackedState>(2, opt, StandardNovelty::R_0);
    }
    else if (boost::iequals(method, "bfws1-rx")) {
        if (using_ext_state) return new BreadthFirstWidthSearch<ExtensionalPackedState>(1, opt, StandardNovelty::R_X);
        else return new BreadthFirstWidthSearch<SparsePackedState>(1, opt, StandardNovelty::R_X);
    }
    else if (boost::iequals(method, "bfws2-rx")) {
        if (using_ext_state) return new BreadthFirstWidthSearch<ExtensionalPackedState>(2, opt, StandardNovelty::R_X);
        else return new BreadthFirstWidthSearch<SparsePackedState>(2, opt, StandardNovelty::R_X);
    }
    else if (boost::iequals(method, "iw1")) {
        if (using_ext_state) return new BreadthFirstWidthSearch<ExtensionalPackedState>(1, opt, StandardNovelty::IW);
        else return new BreadthFirstWidthSearch<SparsePackedState>(1, opt, StandardNovelty::IW);
    }
    else if (boost::iequals(method, "iw2")) {
        if (using_ext_state) return new BreadthFirstWidthSearch<ExtensionalPackedState>(2, opt, StandardNovelty::IW);
        else return new BreadthFirstWidthSearch<SparsePackedState>(2, opt, StandardNovelty::IW);
    }
    else if (boost::iequals(method, "iw1gc")) {
        if (using_ext_state) return new BreadthFirstWidthSearch<ExtensionalPackedState>(1, opt, StandardNovelty::IW_G);
        else return new BreadthFirstWidthSearch<SparsePackedState>(1, opt, StandardNovelty::IW_G);
    }
    else if (boost::iequals(method, "iw2gc")) {
        if (using_ext_state) return new BreadthFirstWidthSearch<ExtensionalPackedState>(2, opt, StandardNovelty::IW_G);
        else return new BreadthFirstWidthSearch<SparsePackedState>(2, opt, StandardNovelty::IW_G);
    }
    else if (boost::iequals(method, "dq-bfws1-rx")) {
        if (using_ext_state) return new DualQueueBFWS<ExtensionalPackedState>(1, opt);
        else return new DualQueueBFWS<SparsePackedState>(1, opt);
    }
    else if (boost::iequals(method, "dq-bfws2-rx")) {
        if (using_ext_state) return new DualQueueBFWS<ExtensionalPackedState>(2, opt);
        else return new DualQueueBFWS<SparsePackedState>(2, opt);
    }
    else if (boost::iequals(method, "alt-bfws1")) {
        if (using_ext_state) return new AlternatedBFWS<ExtensionalPackedState>(1, opt);
        else return new AlternatedBFWS<SparsePackedState>(1, opt);
    }
    else if (boost::iequals(method, "alt-bfws2")) {
        if (using_ext_state) return new AlternatedBFWS<ExtensionalPackedState>(2, opt);
        else return new AlternatedBFWS<SparsePackedState>(2, opt);
    }
    else if (boost::iequals(method, "gbfs")) {
        if (using_ext_state) return new GreedyBestFirstSearch<ExtensionalPackedState>();
        else return new GreedyBestFirstSearch<SparsePackedState>();
    }
    else if (boost::iequals(method, "lazy")) {
        if (using_ext_state) return new LazySearch<ExtensionalPackedState>(true, false);
        else return new LazySearch<SparsePackedState>(true, false);
    }
    else if (boost::iequals(method, "lazy-po")) {
        if (using_ext_state) return new LazySearch<ExtensionalPackedState>(false, false);
        else return new LazySearch<SparsePackedState>(false, false);
    }
    else if (boost::iequals(method, "lazy-prune")) {
        if (using_ext_state) return new LazySearch<ExtensionalPackedState>(false, true);
        else return new LazySearch<SparsePackedState>(false, true);
    }
    else {
        std::cerr << "Invalid search method \"" << method << "\"" << std::endl;
        exit(-1);
    }
}
