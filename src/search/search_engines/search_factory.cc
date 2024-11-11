
#include "search_factory.h"

#include "astar_search.h"
#include "alternated_bfws.h"
#include "breadth_first_search.h"
#include "best_first_width_search.h"
#include "dual_queue_bfws.h"
#include "greedy_best_first_search.h"
#include "lazy_search.h"
#include "search.h"

#include "../states/sparse_states.h"

#include <boost/algorithm/string.hpp>

SearchBase*
SearchFactory::create(const Options &opt, const std::string& method) {
    std::cout << "Creating search factory for method " << method << "..." << std::endl;

    if (boost::iequals(method, "astar")) {
        return new AStarSearch<SparsePackedState>();
    }
    else if (boost::iequals(method, "bfs")) {
        return new BreadthFirstSearch<SparsePackedState>();
    }
    else if (boost::iequals(method, "bfws1")) {
        return new BestFirstWidthSearch<SparsePackedState>(1, opt, StandardNovelty::R_0);
    }
    else if (boost::iequals(method, "bfws2")) {
        return new BestFirstWidthSearch<SparsePackedState>(2, opt, StandardNovelty::R_0);
    }
    else if (boost::iequals(method, "bfws1-rx")) {
        return new BestFirstWidthSearch<SparsePackedState>(1, opt, StandardNovelty::R_X);
    }
    else if (boost::iequals(method, "bfws2-rx")) {
        return new BestFirstWidthSearch<SparsePackedState>(2, opt, StandardNovelty::R_X);
    }
    else if (boost::iequals(method, "iw1")) {
        return new BestFirstWidthSearch<SparsePackedState>(1, opt, StandardNovelty::IW);
    }
    else if (boost::iequals(method, "iw2")) {
        return new BestFirstWidthSearch<SparsePackedState>(2, opt, StandardNovelty::IW);
    }
    else if (boost::iequals(method, "iw1gc")) {
        return new BestFirstWidthSearch<SparsePackedState>(1, opt, StandardNovelty::IW_G);
    }
    else if (boost::iequals(method, "iw2gc")) {
        return new BestFirstWidthSearch<SparsePackedState>(2, opt, StandardNovelty::IW_G);
    }
    else if (boost::iequals(method, "dq-bfws1-rx")) {
        return new DualQueueBFWS<SparsePackedState>(1, opt);
    }
    else if (boost::iequals(method, "dq-bfws2-rx")) {
        return new DualQueueBFWS<SparsePackedState>(2, opt);
    }
    else if (boost::iequals(method, "alt-bfws1")) {
        return new AlternatedBFWS<SparsePackedState>(1, opt);
    }
    else if (boost::iequals(method, "alt-bfws2")) {
        return new AlternatedBFWS<SparsePackedState>(2, opt);
    }
    else if (boost::iequals(method, "gbfs")) {
        return new GreedyBestFirstSearch<SparsePackedState>();
    }
    else if (boost::iequals(method, "lazy")) {
        return new LazySearch<SparsePackedState>(true, false);
    }
    else if (boost::iequals(method, "lazy-po")) {
        return new LazySearch<SparsePackedState>(false, false);
    }
    else if (boost::iequals(method, "lazy-prune")) {
        return new LazySearch<SparsePackedState>(false, true);
    }
    else {
        std::cerr << "Invalid search method \"" << method << "\"" << std::endl;
        exit(-1);
    }
}
