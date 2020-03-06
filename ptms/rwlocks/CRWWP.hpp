/*
 * Copyright 2014-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _CRWWP_H_
#define _CRWWP_H_

#include <atomic>
#include <stdexcept>
#include <cstdint>
#include <thread>


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
class CRWWP {

private:
    class TicketLock {
        alignas(128) std::atomic<uint64_t> ticket {0};
        alignas(128) std::atomic<uint64_t> grant {0};
    public:
        bool isLocked() { return grant.load(std::memory_order_acquire) != ticket.load(std::memory_order_acquire); }
        void lock() {
            auto tkt = ticket.fetch_add(1);
            while (tkt != grant.load(std::memory_order_acquire)) Pause();
        }
        void unlock() {
            auto tkt = grant.load(std::memory_order_relaxed);
            grant.store(tkt+1, std::memory_order_release);
        }
    };

    class RIAtomicCounterArray {
    private:
        static const int MAX_THREADS = 64;
        static const int CLPAD = (128/sizeof(std::atomic<uint64_t>));
        static const int COUNTER_SIZE = 3*MAX_THREADS; // Alternatively, use std::thread::hardware_concurrency()
        std::hash<std::thread::id> hashFunc {};
        alignas(128) std::atomic<uint64_t> counters[COUNTER_SIZE*CLPAD] ;
    public:
        RIAtomicCounterArray() {
            for (int i=0; i < COUNTER_SIZE; i++) {
                counters[i*CLPAD].store(0, std::memory_order_relaxed);
            }
        }
        void arrive(const int notused=0) noexcept {
            const uint64_t tid = hashFunc(std::this_thread::get_id());
            const int icounter = (int)(tid % COUNTER_SIZE);
            counters[icounter*CLPAD].fetch_add(1);
        }
        void depart(const int notused=0) noexcept {
            const uint64_t tid = hashFunc(std::this_thread::get_id());
            const int icounter = (int)(tid % COUNTER_SIZE);
            counters[icounter*CLPAD].fetch_add(-1);
        }
        bool isEmpty(void) noexcept {
            for (int i = 0; i < COUNTER_SIZE; i++) {
                if (counters[i*CLPAD].load(std::memory_order_acquire) > 0) return false;
            }
            return true;
        }
    };

    static const int MAX_THREADS = 128;
    //static const int LOCKED = 1;
    //static const int UNLOCKED = 0;
    const int maxThreads;
    RIAtomicCounterArray ri {};
    //alignas(128) std::atomic<int> cohort { UNLOCKED };
    TicketLock cohort {};

public:
    CRWWP(const int maxThreads=MAX_THREADS) : maxThreads{maxThreads} { }

    std::string className() { return "C-RW-WP"; }

    void exclusiveLock() {
        cohort.lock();
        while (!ri.isEmpty()) Pause();
    }

    void exclusiveUnlock() {
        cohort.unlock();
    }

    void sharedLock() {
        while (true) {
            ri.arrive();
            if (!cohort.isLocked()) break;
            ri.depart();
            while (cohort.isLocked()) Pause();
        }
    }

    void sharedUnlock() {
        ri.depart();
    }
};

#endif /* _CRWWP_H_ */
