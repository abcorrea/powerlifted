#include <iostream>
#include <fstream>

#include "utils.h"

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