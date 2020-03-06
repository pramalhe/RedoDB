/*
 * Copyright 2014-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */

#ifndef _HAZARD_POINTERS_CXPUC_H_
#define _HAZARD_POINTERS_CXPUC_H_

#include <atomic>
#include <iostream>
#include <vector>

namespace cxpuc{
/**
 * The only differences between HP and HP CX is the check on obj->refcnt and obj->next being self-linked in retired()
 *
 */
template<typename T>
class HazardPointersCX {

private:
    static const int      HP_MAX_THREADS = 128;
    static const int      HP_MAX_HPS = 5;     // This is named 'K' in the HP paper
    static const int      CLPAD = 128/sizeof(std::atomic<T*>);
    static const int      HP_THRESHOLD_R = 0; // This is named 'R' in the HP paper
    static const int      MAX_RETIRED = HP_MAX_THREADS*HP_MAX_HPS; // Maximum number of retired objects per thread

    const int             maxHPs;
    const int             maxThreads;

    alignas(128) std::atomic<T*>*      hp[HP_MAX_THREADS];
    // It's not nice that we have a lot of empty vectors, but we need padding to avoid false sharing
    alignas(128) std::vector<T*>       retiredList[HP_MAX_THREADS*CLPAD];

public:
    HazardPointersCX(int maxHPs=HP_MAX_HPS, int maxThreads=HP_MAX_THREADS) : maxHPs{maxHPs}, maxThreads{maxThreads} {
        for (int it = 0; it < HP_MAX_THREADS; it++) {
            hp[it] = new std::atomic<T*>[CLPAD*2]; // We allocate four cache lines to allow for many hps and without false sharing
            retiredList[it*CLPAD].reserve(MAX_RETIRED);
            for (int ihp = 0; ihp < HP_MAX_HPS; ihp++) {
                hp[it][ihp].store(nullptr, std::memory_order_relaxed);
            }
        }
    }

    ~HazardPointersCX() {
        for (int it = 0; it < HP_MAX_THREADS; it++) {
            delete[] hp[it];
            // Clear the current retired nodes
            for (unsigned iret = 0; iret < retiredList[it*CLPAD].size(); iret++) {
                delete retiredList[it*CLPAD][iret];
            }
        }
    }


    /**
     * Progress Condition: wait-free bounded (by maxHPs)
     */
    inline void clear(const int tid) {
        for (int ihp = 0; ihp < maxHPs; ihp++) {
            hp[tid][ihp].store(nullptr, std::memory_order_release);
        }
    }


    /**
     * Progress Condition: wait-free population oblivious
     */
    inline void clearOne(int ihp, const int tid) {
        hp[tid][ihp].store(nullptr, std::memory_order_release);
    }


    /**
     * Progress Condition: lock-free
     */
    inline T* protect(int index, const std::atomic<T*>& atom, const int tid) {
        T* n = nullptr;
        T* ret;
		while ((ret = atom.load()) != n) {
			hp[tid][index].store(ret);
			n = ret;
		}
		return ret;
    }

	
    /**
     * This returns the same value that is passed as ptr, which is sometimes useful
     * Progress Condition: wait-free population oblivious
     */
    inline T* protectPtr(int index, T* ptr, const int tid) {
        hp[tid][index].store(ptr);
        return ptr;
    }



    /**
     * This returns the same value that is passed as ptr, which is sometimes useful
     * Progress Condition: wait-free population oblivious
     */
    inline T* protectPtrRelease(int index, T* ptr, const int tid) {
        hp[tid][index].store(ptr, std::memory_order_release);
        return ptr;
    }


    /**
     * Progress Condition: wait-free bounded (by the number of threads squared)
     *
     * The only differences between HP and HP CX is the check on obj->refcnt and obj->next being self-linked
     */
    void retire(T* ptr, const int tid) {
        retiredList[tid*CLPAD].push_back(ptr);
        if (retiredList[tid*CLPAD].size() < HP_THRESHOLD_R) return;
        for (unsigned iret = 0; iret < retiredList[tid*CLPAD].size(); iret++) {
            auto obj = retiredList[tid*CLPAD][iret];
            if (obj->next.load() != obj) continue; // Delete only if node.next == node
            bool canDelete = true;
            for (int it = 0; it < maxThreads && canDelete; it++) {
                for (int ihp = 0; ihp < maxHPs; ihp++) {
                    if (hp[it][ihp].load() == obj) {
                        canDelete = false;
                        break;
                    }
                }
            }
            if (canDelete && obj->refcnt.load() == 0) { // Delete only if ORC is zero
                retiredList[tid*CLPAD].erase(retiredList[tid*CLPAD].begin() + iret);
                delete obj;
                iret--;
                continue;
            }
        }
    }
};
}
#endif /* _HAZARD_POINTERS_CXPUC_H_ */
