#include "utils.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <limits>

using namespace std;

int get_peak_memory_in_kb() {
    // DISCLAIMER: code from Fast Downward src/search/utils/system_unix.cc

    // On error, produces a warning on cerr and returns -1.
    int memory_in_kb = -1;

#if OPERATING_SYSTEM == LINUX
    ifstream procfile;
    procfile.open("/proc/self/status");
    string word;
    while (procfile.good()) {
        procfile >> word;
        if (word == "VmPeak:") {
            procfile >> memory_in_kb;
            break;
        }
        // Skip to end of line.
        procfile.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    if (procfile.fail())
        memory_in_kb = -1;
#endif
    if (memory_in_kb == -1)
        cerr << "warning: could not determine peak memory" << endl;
    return memory_in_kb;

}

bool is_product_within_limit(int factor1, int factor2, int limit) {
    assert(factor1 >= 0);
    assert(factor2 >= 0);
    assert(limit >= 0);
    return factor2 == 0 || factor1 <= limit / factor2;
}

static bool is_product_within_limit_unsigned(
        unsigned int factor1, unsigned int factor2, unsigned int limit) {
    return factor2 == 0 || factor1 <= limit / factor2;
}

static unsigned int safe_abs(int x) {
    // Don't call abs() if the call would overflow.
    if (x == numeric_limits<int>::min()) {
        return static_cast<unsigned int>(-(x + 1)) + 1u;
    }
    return abs(x);
}

bool is_product_within_limits(
        int factor1, int factor2, int lower_limit, int upper_limit) {
    assert(lower_limit < 0);
    assert(upper_limit >= 0);

    if (factor1 >= 0 && factor2 >= 0) {
        return is_product_within_limit(factor1, factor2, upper_limit);
    } else if (factor1 < 0 && factor2 < 0) {
        return is_product_within_limit_unsigned(
                safe_abs(factor1), safe_abs(factor2), upper_limit);
    } else {
        return is_product_within_limit_unsigned(
                safe_abs(factor1), safe_abs(factor2), safe_abs(lower_limit));
    }
}
