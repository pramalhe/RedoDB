/*
 * Copyright 2014-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _HAZARD_POINTERS_H_
#define _HAZARD_POINTERS_H_

#include <atomic>
#include <iostream>
#include <functional>
#include <vector>


template<typename T>
class HazardPointers {

private:
    static const int      HP_MAX_THREADS = 128;
    static const int      HP_MAX_HPS = 128;     // This is named 'K' in the HP paper
    static const int      CLPAD = 128/sizeof(std::atomic<T*>);
    static const int      HP_THRESHOLD_R = 0; // This is named 'R' in the HP paper
    static const int      MAX_RETIRED = HP_MAX_THREADS*HP_MAX_HPS; // Maximum number of retired objects per thread

    const int             maxHPs;
    const int             maxThreads;

    alignas(128) std::atomic<T*>*      hp[HP_MAX_THREADS];
    // It's not nice that we have a lot of empty vectors, but we need padding to avoid false sharing
    alignas(128) std::vector<T*>       retiredList[HP_MAX_THREADS*CLPAD];
    std::function<void(T*,int)> defdeleter = [](T* t, int tid){ delete t; };
    std::function<void(T*,int)>& deleter;
public:

    HazardPointers(int maxHPs, int maxThreads) : maxHPs{maxHPs}, maxThreads{maxThreads}, deleter{defdeleter} {
        for (int ithread = 0; ithread < HP_MAX_THREADS; ithread++) {
            hp[ithread] = new std::atomic<T*>[HP_MAX_HPS];
            for (int ihp = 0; ihp < HP_MAX_HPS; ihp++) {
                hp[ithread][ihp].store(nullptr, std::memory_order_relaxed);
            }
        }
    }

    HazardPointers(int maxHPs, int maxThreads, std::function<void(T*,int)>& deleter) : maxHPs{maxHPs}, maxThreads{maxThreads}, deleter{deleter} {
        for (int ithread = 0; ithread < HP_MAX_THREADS; ithread++) {
            hp[ithread] = new std::atomic<T*>[HP_MAX_HPS];
            for (int ihp = 0; ihp < HP_MAX_HPS; ihp++) {
                hp[ithread][ihp].store(nullptr, std::memory_order_relaxed);
            }
        }
    }

    ~HazardPointers() {
        for (int ithread = 0; ithread < HP_MAX_THREADS; ithread++) {
            delete[] hp[ithread];
            // Clear the current retired nodes
            for (unsigned iret = 0; iret < retiredList[ithread*CLPAD].size(); iret++) {
                delete retiredList[ithread*CLPAD][iret];
            }
        }
    }


    /**
     * Progress Condition: wait-free bounded (by maxHPs)
     */
    void clear(const int tid) {
        for (int ihp = 0; ihp < maxHPs; ihp++) {
            hp[tid][ihp].store(nullptr, std::memory_order_release);
        }
    }


    /**
     * Progress Condition: wait-free population oblivious
     */
    void clearOne(int ihp, const int tid) {
        hp[tid][ihp].store(nullptr, std::memory_order_release);
    }


    /**
     * Progress Condition: lock-free
     */
    T* protect(int index, const std::atomic<T*>& atom, const int tid) {
        T* n = nullptr;
        T* ret;
		while ((ret = atom.load()) != n) {
			hp[tid][index].store(ret);
			n = ret;
		}
		return ret;
    }

    T* get(int index, const int tid){
        return hp[tid][index].load();
    }
    /**
     * This returns the same value that is passed as ptr, which is sometimes useful
     * Progress Condition: wait-free population oblivious
     */
    T* protectPtr(int index, T* ptr, const int tid) {
        hp[tid][index].store(ptr);
        return ptr;
    }



    /**
     * This returns the same value that is passed as ptr, which is sometimes useful
     * Progress Condition: wait-free population oblivious
     */
    T* protectPtrRelease(int index, T* ptr, const int tid) {
        hp[tid][index].store(ptr, std::memory_order_release);
        return ptr;
    }


    /**
     * Progress Condition: wait-free bounded (by the number of threads squared)
     */
    void retire(T* ptr, const int tid) {
        retiredList[tid*CLPAD].push_back(ptr);
        if (retiredList[tid*CLPAD].size() < HP_THRESHOLD_R) return;
        for (unsigned iret = 0; iret < retiredList[tid*CLPAD].size();) {
            auto obj = retiredList[tid*CLPAD][iret];
            bool canDelete = true;
            for (int tid = 0; tid < maxThreads && canDelete; tid++) {
                for (int ihp = maxHPs-1; ihp >= 0; ihp--) {
                    if (hp[tid][ihp].load() == obj) {
                        canDelete = false;
                        break;
                    }
                }
            }
            if (canDelete) {
                retiredList[tid*CLPAD].erase(retiredList[tid*CLPAD].begin() + iret);
                deleter(obj,tid);
                continue;
            }
            iret++;
        }
    }
};

#endif /* _HAZARD_POINTERS_H_ */
