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
#define DATA_FILE "data/pq-ll-enq-deq-cxptm.txt"
#elif defined USE_CXPUC
#include "ptms/cxpuc/CXPUC.hpp"
#include "ptms/cxpuc/PUCQueue.hpp"
#include "pdatastructures/psequential/PLinkedListQueue.hpp"
#define DATA_FILE "data/pq-ll-enq-deq-cxpuc.txt"
#elif defined USE_REDO
#include "pdatastructures/pqueues/RedoLinkedListQueue.hpp"
#define DATA_FILE "data/pq-ll-enq-deq-redo.txt"
#elif defined USE_REDOTIMED
#include "pdatastructures/pqueues/RedoTimedLinkedListQueue.hpp"
#define DATA_FILE "data/pq-ll-enq-deq-redotimed.txt"
#elif defined USE_REDOOPT
#include "pdatastructures/pqueues/RedoOptLinkedListQueue.hpp"
#define DATA_FILE "data/pq-ll-enq-deq-redoopt.txt"
#elif defined USE_OFWF
#include "pdatastructures/pqueues/POFWFLinkedListQueue.hpp"
#define DATA_FILE "data/pq-ll-enq-deq-ofwf.txt"
#elif defined USE_PMDK
#include "pdatastructures/pqueues/PMDKLinkedListQueue.hpp"
#define DATA_FILE "data/pq-ll-enq-deq-pmdk.txt"
#elif defined USE_FRIEDMAN
#include "pdatastructures/pqueues/PFriedmanQueue.hpp"
#define DATA_FILE "data/pq-ll-enq-deq-friedman.txt"
#elif defined USE_NORMALIZED_OPT
#include "pdatastructures/pqueues/NormalQueue-opt.hpp"
#define DATA_FILE "data/pq-ll-enq-deq-normopt.txt"
#elif defined USE_FRIEDMAN_VOLATILE
#include "pdatastructures/pqueues/PFriedmanQueue.hpp"
#define DATA_FILE "data/pq-ll-enq-deq-friedman-volatile.txt"
#elif defined USE_NORMALIZED_OPT_VOLATILE
#include "pdatastructures/pqueues/NormalQueue-opt.hpp"
#define DATA_FILE "data/pq-ll-enq-deq-normopt-volatile.txt"
#elif defined USE_REDOOPT_VOLATILE
#include "pdatastructures/pqueues/RedoOptLinkedListQueue.hpp"
#define DATA_FILE "data/pq-ll-enq-deq-redoopt-volatile.txt"
#endif
#include "PBenchmarkQueues.hpp"


// Used only if MEASURE_PWB is defined and PMDK allocator is not being used
thread_local uint64_t tl_num_pwbs = 0;
thread_local uint64_t tl_num_pfences = 0;


int main(void) {
    std::string dataFilename { DATA_FILE };
#ifdef MEASURE_PWB // Change the name of the .txt output file
    dataFilename.replace(8, 10, "pwb");
#endif
    vector<int> threadList = { 1, 2, 4, 8, 16, 24, 32, 40}; // For castor-1
    const int numPairs = 10*1000*1000;                               // Number of pairs of items to enqueue-dequeue. 10M for the paper
    const int numRuns = 1;                                           // 5 runs for the paper
    const int EMAX_CLASS = 10;
    uint64_t results[EMAX_CLASS][threadList.size()];
    std::string cNames[EMAX_CLASS];
    int maxClass = 0;
    // Reset results
    std::memset(results, 0, sizeof(uint64_t)*EMAX_CLASS*threadList.size());
    //std::cout << "If you use PMDK, don't forget to set 'export PMEM_IS_PMEM_FORCE=1'\n";

#if defined (USE_FRIEDMAN) || defined (USE_NORMALIZED_OPT)
    std::cout << "FHMP and NormalizedOpt are using PMDK allocator.\n\n";
#elif defined USE_FRIEDMAN_VOLATILE || USE_NORMALIZED_OPT_VOLATILE
    std::cout << "If you're on Optane (castor-1) do the following:\n";
    std::cout << "export VMMALLOC_POOL_SIZE=$((64 * 1024 * 1024 *1024))\n";
    std::cout << "export VMMALLOC_POOL_DIR=\"/mnt/pmem0\"\n";
    std::cout << "LD_PRELOAD=\"/usr/local/lib/libvmmalloc.so\" bin/pq-ll-enq-deq-friedman-volatile\n";
    std::cout << "LD_PRELOAD=\"/usr/local/lib/libvmmalloc.so\" bin/pq-ll-enq-deq-normopt-volatile\n";
#endif


    for (unsigned it = 0; it < threadList.size(); it++) {
        auto nThreads = threadList[it];
        int ic = 0;
        PBenchmarkQueues bench(nThreads);
        std::cout << "\n----- Persistent Queues (Linked-Lists)   numPairs=" << numPairs << "   threads=" << nThreads << "   runs=" << numRuns << " -----\n";
#ifdef USE_CXPTM
        results[ic][it] = bench.enqDeq<CXPTMLinkedListQueue<uint64_t>,cx::CX>                 (cNames[ic], numPairs, numRuns);
        ic++;
#elif defined USE_CXPUC
        results[ic][it] = bench.enqDeqPUC<PUCQueue<PLinkedListQueue<uint64_t,cxpuc::Allocator>,uint64_t>> (cNames[ic], numPairs, numRuns);
        ic++;
#elif defined USE_REDO
        results[ic][it] = bench.enqDeq<RedoLinkedListQueue<uint64_t>,redo::Redo>        (cNames[ic], numPairs, numRuns);
        ic++;
#elif defined USE_REDOTIMED
        results[ic][it] = bench.enqDeq<RedoTimedLinkedListQueue<uint64_t>,redotimed::RedoTimed>(cNames[ic], numPairs, numRuns);
        ic++;
#elif defined USE_REDOOPT || defined USE_REDOOPT_VOLATILE
        results[ic][it] = bench.enqDeq<RedoOptLinkedListQueue<uint64_t>,redoopt::RedoOpt>(cNames[ic], numPairs, numRuns);
        ic++;
#elif defined USE_OFWF
        results[ic][it] = bench.enqDeq<POFWFLinkedListQueue<uint64_t>,onefileptmwf::OneFileWF>(cNames[ic], numPairs, numRuns);
        ic++;
#elif defined USE_PMDK
        results[ic][it] = bench.enqDeq<PMDKLinkedListQueue<uint64_t>,pmdk::PMDKTM>            (cNames[ic], numPairs, numRuns);
        ic++;
#elif defined USE_FRIEDMAN || defined USE_FRIEDMAN_VOLATILE
        results[ic][it] = bench.enqDeqNoTransaction<PFriedmanQueue<uint64_t>>                 (cNames[ic], numPairs, numRuns);
        ic++;
#elif defined USE_NORMALIZED_OPT || defined USE_NORMALIZED_OPT_VOLATILE
        results[ic][it] = bench.enqDeqNoTransaction<NormalQueueOpt<uint64_t>>                 (cNames[ic], numPairs, numRuns);
        ic++;
#endif
        maxClass = ic;
    }

    // Export tab-separated values to a file to be imported in gnuplot or excel
    ofstream dataFile;
    dataFile.open(dataFilename);
    dataFile << "Threads\t";
    // Printf class names
    for (int ic = 0; ic < maxClass; ic++) dataFile << cNames[ic] << "\t";
    dataFile << "\n";
    for (int it = 0; it < threadList.size(); it++) {
        dataFile << threadList[it] << "\t";
        for (int ic = 0; ic < maxClass; ic++) dataFile << results[ic][it] << "\t";
        dataFile << "\n";
    }
    dataFile.close();
    std::cout << "\nSuccessfuly saved results in " << dataFilename << "\n";

    return 0;
}
