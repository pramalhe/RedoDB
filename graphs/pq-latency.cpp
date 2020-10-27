/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#include <iostream>
#include <fstream>
#include <cstring>

#ifdef USE_CXPTM
#include "pdatastructures/pqueues/CXPTMLinkedListQueue.hpp"
#elif defined USE_REDO
#include "pdatastructures/pqueues/RedoLinkedListQueue.hpp"
#elif defined USE_REDOTIMED
#include "pdatastructures/pqueues/RedoTimedLinkedListQueue.hpp"
#elif defined USE_REDOOPT
#include "pdatastructures/pqueues/RedoOptLinkedListQueue.hpp"
#elif defined USE_OFWF
#include "pdatastructures/pqueues/POFWFLinkedListQueue.hpp"
#elif defined USE_PMDK
#include "pdatastructures/pqueues/PMDKLinkedListQueue.hpp"
#elif defined USE_FRIEDMAN
#include "pdatastructures/pqueues/PFriedmanQueue.hpp"
#elif defined USE_NORMALIZED_OPT
#include "pdatastructures/pqueues/NormalQueue-opt.hpp"
#endif
#include "PBenchmarkLatencyQueues.hpp"


int main(void) {
    vector<int> threadList = { 16 }; // For castor-1, so we stay below the number of physical cores
    const int numRuns = 1;                                           // 5 runs for the paper
    std::string cName;


#if defined (USE_FRIEDMAN) || defined (USE_NORMALIZED_OPT)
    std::cout << "FHMP and NormalizedOpt are using PMDK allocator.\n\n";
#endif

    for (unsigned it = 0; it < threadList.size(); it++) {
        auto nThreads = threadList[it];
        PBenchmarkLatencyQueues bench(nThreads);
        std::cout << "\n----- Latency for Persistent Queues (Linked-Lists)   threads=" << nThreads << " -----\n";
#ifdef USE_CXPTM
        bench.latencyBurstBenchmark<CXPTMLinkedListQueue<uint64_t>,cx::CX>(cName);
#elif defined USE_REDO
        bench.latencyBurstBenchmark<RedoLinkedListQueue<uint64_t>,redo::Redo>(cName);
#elif defined USE_REDOTIMED
        bench.latencyBurstBenchmark<RedoTimedLinkedListQueue<uint64_t>,redotimed::RedoTimed>(cName);
#elif defined USE_REDOOPT
        bench.latencyBurstBenchmark<RedoOptLinkedListQueue<uint64_t>,redoopt::RedoOpt>(cName);
#elif defined USE_OFWF
        bench.latencyBurstBenchmark<POFWFLinkedListQueue<uint64_t>,onefileptmwf::OneFileWF>(cName);
#elif defined USE_PMDK
        bench.latencyBurstBenchmark<PMDKLinkedListQueue<uint64_t>,pmdk::PMDKTM>(cName);
#elif defined USE_FRIEDMAN || defined USE_FRIEDMAN_VOLATILE
        bench.latencyBurstBenchmark<PFriedmanQueue<uint64_t>>(cName);
#elif defined USE_NORMALIZED_OPT || defined USE_NORMALIZED_OPT_VOLATILE
        bench.latencyBurstBenchmark<NormalQueueOpt<uint64_t>>(cName);
#endif
    }

    return 0;
}
