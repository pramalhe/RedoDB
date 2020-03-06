/*
 * Copyright 2014-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */

#ifndef _CRWWP_SPIN_LOCK_H_
#define _CRWWP_SPIN_LOCK_H_

#include <atomic>
#include <stdexcept>
#include <cstdint>
#include <thread>

#include "../common/ThreadRegistry.hpp"

// This class is used by RomulusLog


// Pause to prevent excess processor bus usage
#if defined( __sparc )
#define Pause() __asm__ __volatile__ ( "rd %ccr,%g0" )
#elif defined( __i386 ) || defined( __x86_64 )
#define Pause() __asm__ __volatile__ ( "pause" : : : )
#else
#define Pause() std::this_thread::yield();
#endif




/**
 * <h1> C-RW-WP </h1>
 *
 * A C-RW-WP reader-writer lock with writer preference and using a
 * Ticket Lock as Cohort.
 * This is starvation-free for writers and for readers, but readers may be
 * starved by writers.
 *
 * C-RW-WP paper:         http://dl.acm.org/citation.cfm?id=2442532
 *
 * <p>
 * @author Pedro Ramalhete
 * @author Andreia Correia
 */
class CRWWPSpinLock {

private:
    class SpinLock {
        alignas(128) std::atomic<int> writers {0};
    public:
        bool isLocked() { return (writers.load()==1); }
        void lock() {
            while (!tryLock()) Pause();
        }
        bool tryLock() {
            if(writers.load()==1)return false;
            int tmp = 0;
            return writers.compare_exchange_strong(tmp,1);
        }
        void unlock() {
            writers.store(0, std::memory_order_release);
        }
    };

    class RIStaticPerThread {
    private:
        static const uint64_t NOT_READING = 0;
        static const uint64_t READING = 1;
        static const int CLPAD = 128/sizeof(uint64_t);
        alignas(128) std::atomic<uint64_t>* states;

    public:
        RIStaticPerThread() {
            states = new std::atomic<uint64_t>[REGISTRY_MAX_THREADS*CLPAD];
            for (int tid = 0; tid < REGISTRY_MAX_THREADS; tid++) {
                states[tid*CLPAD].store(NOT_READING, std::memory_order_relaxed);
            }
        }

        ~RIStaticPerThread() {
            delete[] states;
        }

        inline void arrive(const int tid) noexcept {
            states[tid*CLPAD].store(READING);
        }

        inline void depart(const int tid) noexcept {
            states[tid*CLPAD].store(NOT_READING, std::memory_order_release);
        }

        inline bool isEmpty() noexcept {
            const int maxTid = ThreadRegistry::getMaxThreads();
            for (int tid = 0; tid < maxTid; tid++) {
                if (states[tid*CLPAD].load() != NOT_READING) return false;
            }
            return true;
        }
    };

    RIStaticPerThread ri {};
    SpinLock splock {};

public:
    CRWWPSpinLock() { }

    std::string className() { return "C-RW-WP-SpinLock"; }

    void exclusiveLock() {
        splock.lock();
        while (!ri.isEmpty()) Pause();
    }

    bool tryExclusiveLock() {
        return splock.tryLock();
    }

    void exclusiveUnlock() {
        splock.unlock();
    }

    void sharedLock(const int tid) {
        while (true) {
            ri.arrive(tid);
            if (!splock.isLocked()) break;
            ri.depart(tid);
            while (splock.isLocked()) Pause();
        }
    }

    void sharedUnlock(const int tid) {
        ri.depart(tid);
    }

    void waitForReaders(){
        while (!ri.isEmpty()) {} // spin waiting for readers
    }
};

#endif /* _CRWWP_H_ */
