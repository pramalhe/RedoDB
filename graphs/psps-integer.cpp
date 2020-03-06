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
#include "ptms/cxptm/CXPTM.hpp"
#define DATA_FILE "data/psps-integer-cxptm.txt"
#elif defined USE_CXPUC
#error "SPS not (yet) implemented for CX-PUC"
#include "ptms/cxpuc/CXPUC.hpp"
#define DATA_FILE "data/psps-integer-cxpuc.txt"
#elif defined USE_REDO
#include "ptms/redo/Redo.hpp"
#define DATA_FILE "data/psps-integer-redo.txt"
#elif defined USE_REDOTIMED
#include "ptms/redotimed/RedoTimed.hpp"
#define DATA_FILE "data/psps-integer-redotimed.txt"
#elif defined USE_REDOOPT
#include "ptms/redoopt/RedoOpt.hpp"
#define DATA_FILE "data/psps-integer-redoopt.txt"
#elif defined USE_OFWF
#include "ptms/ponefilewf/OneFilePTMWF.hpp"
#define DATA_FILE "data/psps-integer-ofwf.txt"
#elif defined USE_PMDK
#include "ptms/pmdk/PMDKTM.hpp"
#define DATA_FILE "data/psps-integer-pmdk.txt"
#endif
#include "PBenchmarkSPS.hpp"



int main(void) {
    const std::string dataFilename { DATA_FILE };
    vector<int> threadList = { 1, 2, 4, 8, 16, 24, 32, 40};     // For castor-1
    //vector<long> swapsPerTxList = { 1, 4, 8, 16, 32, 64, 128, 256 };
    vector<long> swapsPerTxList = { 1, 4, 8, 16, 32, 64 };
    const int numRuns = 1;                                   // 5 runs for the paper
    const seconds testLength = 20s;                          // 20s for the paper
    const int EMAX_CLASS = 10;
    int maxClass = 0;
    uint64_t results[EMAX_CLASS][threadList.size()][swapsPerTxList.size()];
    std::string cNames[EMAX_CLASS];
    // Reset results
    std::memset(results, 0, sizeof(uint64_t)*EMAX_CLASS*threadList.size()*swapsPerTxList.size());

    // SPS Benchmarks multi-threaded
#ifdef USE_PMDK
    std::cout << "If you use PMDK on /dev/shm/, don't forget to set 'export PMEM_IS_PMEM_FORCE=1'\n";
#endif
    std::cout << "\n----- Persistent SPS Benchmark (multi-threaded integer array swap) -----\n";
    PBenchmarkSPS bench;
    for (int it = 0; it < threadList.size(); it++) {
        int nThreads = threadList[it];
        for (int is = 0; is < swapsPerTxList.size(); is++) {
            int nWords = swapsPerTxList[is];
            int ic = 0;
            std::cout << "\n----- threads=" << nThreads << "   runs=" << numRuns << "   length=" << testLength.count() << "s   arraySize=" << arraySize << "   swaps/tx=" << nWords << " -----\n";
#ifdef USE_CXPTM
            results[ic][it][is] = bench.benchmarkSPSInteger<cx::CX,                  cx::persist>          (cNames[ic], testLength, nWords, numRuns, nThreads);
            ic++;
#elif defined USE_CXPUC
            results[ic][it][is] = bench.benchmarkSPSInteger<cxpuc::CX,               cxpuc::persist>       (cNames[ic], testLength, nWords, numRuns, nThreads);
            ic++;
#elif defined USE_REDO
            results[ic][it][is] = bench.benchmarkSPSInteger<redo::Redo, redo::persist>      (cNames[ic], testLength, nWords, numRuns, nThreads);
            ic++;
#elif defined USE_REDOTIMED
            results[ic][it][is] = bench.benchmarkSPSInteger<redotimed::RedoTimed, redotimed::persist>      (cNames[ic], testLength, nWords, numRuns, nThreads);
            ic++;
#elif defined USE_REDOOPT
            results[ic][it][is] = bench.benchmarkSPSInteger<redoopt::RedoOpt, redoopt::persist>      (cNames[ic], testLength, nWords, numRuns, nThreads);
            ic++;
#elif defined USE_OFWF
            results[ic][it][is] = bench.benchmarkSPSInteger<onefileptmwf::OneFileWF, onefileptmwf::tmtype> (cNames[ic], testLength, nWords, numRuns, nThreads);
            ic++;
#elif defined USE_PMDK
            results[ic][it][is] = bench.benchmarkSPSInteger<pmdk::PMDKTM,            pmdk::persist>        (cNames[ic], testLength, nWords, numRuns, nThreads);
            ic++;
#endif
            maxClass = ic;
        }
        std::cout << "\n";
    }

    // Export tab-separated values to a file to be imported in gnuplot or excel
    ofstream dataFile;
    dataFile.open(dataFilename);
    dataFile << "Threads\t";
    // Printf class names for each column plus the corresponding thread
    for (int ic = 0; ic < maxClass; ic++) {
        for (unsigned is = 0; is < swapsPerTxList.size(); is++) {
            int nWords = swapsPerTxList[is];
            dataFile << cNames[ic] << "-" << nWords <<"\t";
        }
    }
    dataFile << "\n";
    for (unsigned it = 0; it < threadList.size(); it++) {
        dataFile << threadList[it] << "\t";
        for (int ic = 0; ic < maxClass; ic++) {
            for (unsigned is = 0; is < swapsPerTxList.size(); is++) {
                dataFile << results[ic][it][is] << "\t";
            }
        }
        dataFile << "\n";
    }
    dataFile.close();
    std::cout << "\nSuccessfuly saved results in " << dataFilename << "\n";



    return 0;
}
