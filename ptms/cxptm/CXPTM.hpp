/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _CX_PERSISTENT_TRANSACTIONAL_MEMORY_H_
#define _CX_PERSISTENT_TRANSACTIONAL_MEMORY_H_

#include <atomic>
#include <cstdint>
#include <cassert>
#include <string>
#include <cstring>      // std::memcpy()
#include <sys/mman.h>   // Needed if we use mmap()
#include <sys/types.h>  // Needed by open() and close()
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>     // Needed by close()
#include <linux/mman.h> // Needed by MAP_SHARED_VALIDATE
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <functional>
#include <set>          // Needed by allocation statistics

#include "../../common/StrongTryRIRWLock.hpp"
#include "../../common/pfences.h"
#include "../../common/ThreadRegistry.hpp"
#include "../cxptm/CircularArray.hpp"
#include "../cxptm/HazardPointersCX.hpp"

// Comment this out if we decide to go with Doug Lea's malloc for CX
#define USE_ESLOCO

// Size of the persistent memory region
#ifndef PM_REGION_SIZE
#define PM_REGION_SIZE (2*1024*1024*1024ULL) // 2GB by default (to run on laptop)
#endif
// DAX flag (MAP_SYNC) is needed for Optane but not for /dev/shm/
#ifdef PM_USE_DAX
#define PM_FLAGS       MAP_SYNC
#else
#define PM_FLAGS       0
#endif
// Name of persistent file mapping
#ifndef PM_FILE_NAME
#define PM_FILE_NAME   "/dev/shm/cx_shared"
#endif

namespace cx {


// Forward declaration of CX to create a global instance
class CX;
extern CX gCX;

// Global with the 'main' size. Used by pload()
extern uint64_t g_main_size;
// Global with the 'main' addr. Used by pload()
extern uint8_t* g_main_addr;
extern uint8_t* g_main_addr_end;
extern uint8_t* g_region_end;

// Counter of nested write transactions
extern thread_local int64_t tl_nested_write_trans;
// Counter of nested read-only transactions
extern thread_local int64_t tl_nested_read_trans;

extern thread_local uint64_t tl_cx_size;

#define ADDR_IS_IN_MAIN(addr) ((addr) >= g_main_addr && (addr) < g_main_addr_end)
#define ADDR_IS_IN_REGION(addr) ((addr) >= g_main_addr && (addr) < g_region_end)

#ifdef USE_ESLOCO
#include "../../common/EsLoco.hpp"
#else // Use Doug Lea's malloc
// This macro is used in Doug Lea's malloc to adjust weird castings that don't call the '&' operator
#define CX_ADJUSTED_ADDR(addr) (ADDR_IS_IN_REGION(addr)) ? (malloc_chunk*)(addr - tl_cx_size) : (malloc_chunk*)addr

typedef void* mspace;
extern void* mspace_malloc(mspace msp, size_t bytes);
extern void mspace_free(mspace msp, void* mem);
extern mspace create_mspace_with_base(void* base, size_t capacity, int locked);
#endif

// Log of the deferred PWBs
#define PWB_LOG_SIZE  256
extern thread_local void* tl_pwb_log[PWB_LOG_SIZE];
extern thread_local int tl_pwb_idx;
#define DEFER_PWB(_addr) if (tl_pwb_idx < PWB_LOG_SIZE) { tl_pwb_log[tl_pwb_idx++] = (_addr); } else { PWB(_addr); }
// Returns the cache line of the address (this is for x86 only)
#define ADDR2CL(_addr) (uint8_t*)((size_t)(_addr) & (~63ULL))


/*
 * Definition of persist<> type.
 * In CX we interpose stores and loads to adjust the synthetic pointers.
 */
template<typename T>
struct persist {
    // Stores the actual value
    T val;

    persist() { }

    persist(T initVal) {
        pstore(initVal);
    }

    // Casting operator
    operator T() {
        return pload();
    }

    // Casting to const
    operator T() const { return pload(); }

    // Prefix increment operator: ++x
    void operator++ () {
        pstore(pload()+1);
    }

    // Prefix decrement operator: --x
    void operator-- () {
        pstore(pload()-1);
    }

    void operator++ (int) {
        pstore(pload()+1);
    }

    void operator-- (int) {
        pstore(pload()-1);
    }

    // Equals operator: first downcast to T and then compare
    bool operator == (const T& otherval) const {
        return pload() == otherval;
    }

    // Difference operator: first downcast to T and then compare
    bool operator != (const T& otherval) const {
        return pload() != otherval;
    }

    // Relational operators
    bool operator < (const T& rhs) {
        return pload() < rhs;
    }
    bool operator > (const T& rhs) {
        return pload() > rhs;
    }
    bool operator <= (const T& rhs) {
        return pload() <= rhs;
    }
    bool operator >= (const T& rhs) {
        return pload() >= rhs;
    }

    T operator % (const T& rhs) {
        return pload() % rhs;
    }

    // Operator arrow ->
    T operator->() {
        return pload();
    }

    // Copy constructor
    persist<T>(const persist<T>& other) {
        pstore(other.pload());
    }

    // Assignment operator from an atomic_mwc
    persist<T>& operator=(const persist<T>& other) {
        pstore(other.pload());
        return *this;
    }

    // Assignment operator from a value
    persist<T>& operator=(T value) {
        pstore(value);
        return *this;
    }

    persist<T>& operator&=(T value) {
        pstore(pload() & value);
        return *this;
    }

    persist<T>& operator|=(T value) {
        pstore(pload() | value);
        return *this;
    }
    persist<T>& operator+=(T value) {
        pstore(pload() + value);
        return *this;
    }
    persist<T>& operator-=(T value) {
        pstore(pload() - value);
        return *this;
    }

    // Operator &
    inline T* operator&() {
        uint8_t* valaddr = (uint8_t*)&val;
        if (ADDR_IS_IN_REGION(valaddr) && !ADDR_IS_IN_MAIN(valaddr)) {
            return reinterpret_cast<T*>( valaddr - tl_cx_size );
        }
        return &val;
    }

    inline void pstore(T newVal) {
        uint8_t* valaddr = (uint8_t*)&val;
        if (tl_cx_size != 0 && ADDR_IS_IN_MAIN(valaddr)) {
            *reinterpret_cast<T*>( valaddr + tl_cx_size ) = newVal;
            DEFER_PWB(ADDR2CL(valaddr + tl_cx_size));
        } else {
            val = newVal;
            if (ADDR_IS_IN_REGION(valaddr)) DEFER_PWB(ADDR2CL(valaddr));
        }
    }

    inline T pload() const {
        uint8_t* valaddr = (uint8_t*)&val;
        const uint64_t cx_size = tl_cx_size;
        if (cx_size != 0 && ADDR_IS_IN_MAIN(valaddr)) {
            return *reinterpret_cast<T*>( valaddr + cx_size );
        } else {
            return val;
        }
    }
};




class CX {

    static const int MAX_READ_TRIES = 10; // Maximum number of times a reader will fail to acquire the shared lock before adding its operation as a mutation
    static const int MAX_COMBINEDS = 128;
    static const int MAX_THREADS = 65;
    static const int NUM_OBJS = 100;
    const int maxThreads;

    struct Node {
        std::function<uint64_t()>  mutation;
        std::atomic<uint64_t>      result;   // This needs to be (relaxed) atomic because there are write-races on it. TODO: change to void*
        std::atomic<bool>          done {false};
        std::atomic<Node*>         next {nullptr};
        std::atomic<uint64_t>      ticket {0};
        std::atomic<int>           refcnt {0};
        const int                  enqTid;

        Node(std::function<uint64_t()>& mutFunc, int tid) : mutation{mutFunc}, enqTid{tid}{ }

        bool casNext(Node* cmp, Node* val) {
            return next.compare_exchange_strong(cmp, val);
        }
    };


    // Class to combine head and the instance.
    // We must separately initialize each of the heaps in the heapPool because
    // they may be aligned differentely. The actual offset that the synthetic
    // pointers must use is not 'heapOffset' but 'ptrOffset', which is calculated
    // based on each Combined's 'ms'.
    struct Combined {
        Node*                      head {nullptr};
        uint8_t*                   root {nullptr}; //offset in bytes
        StrongTryRIRWLock          rwLock {MAX_THREADS};

        // Helper function to update newComb->head while keepting track of ORCs.
        void updateHead(Node* mn) {
            if (mn != nullptr) mn->refcnt.fetch_add(1); // mn is assumed to be protected by an HP
            if (head != nullptr) head->refcnt.fetch_add(-1);
            head = mn;
        }
    };

    std::function<uint64_t()> sentinelMutation = [](){ return 0; };
    Node* sentinel = new Node(sentinelMutation, 0);
    // The tail of the queue/list of mutations.
    // Starts by pointing to a sentinel/dummy node
    std::atomic<Node*> tail {sentinel};

    alignas(128) Combined* combs;

    // Enqueue requests
    alignas(128) std::atomic<Node*> enqueuers[MAX_THREADS];

    // We need two hazard pointers for the enqueue() (ltail and lnext), one for myNode, and two to traverse the list/queue
    HazardPointersCX<Node> hp {5, maxThreads};
    const int kHpTail     = 0;
    const int kHpTailNext = 1;
    const int kHpHead     = 2;
    const int kHpNext     = 3;
    const int kHpMyNode   = 4;

    CircularArray<Node>* preRetired[MAX_THREADS];

    uint64_t getCombined(const uint64_t myTicket, const int tid) {
        for (int i = 0; i < maxThreads; i++) {
            const int curCombIndex = per->curComb.load();
            Combined* lcomb = &combs[curCombIndex];
            // Ensure curComb is persisted before we use lcomb. The CAS in
            // lcomb->rwLock.sharedTryLock(tid) serves as PFENCE.
            PWB(&per->curComb);
            if (!lcomb->rwLock.sharedTryLock(tid)) continue;
            Node* lhead = lcomb->head;
            const uint64_t lticket = lhead->ticket.load();
            if (lticket < myTicket && lhead != lhead->next.load()) return curCombIndex;
            lcomb->rwLock.sharedUnlock(tid);
            // In case lhead->ticket.load() has been made visible
            if (lticket >= myTicket && curCombIndex == per->curComb.load()) return -1;
        }
        return -1;
    }



    // Id for sanity check
    static const uint64_t MAGIC_ID = 0x1337BAB8;

    // Filename for the mapping file
    const char* MMAP_FILENAME = PM_FILE_NAME;

    // Member variables
    bool dommap;
    int fd = -1;
    uint8_t* base_addr;
    uint64_t max_size;

    // One instance of this is at the start of base_addr, in persistent memory
    struct PersistentHeader {
        uint64_t           id {0}; //validation
        std::atomic<int>   curComb {0};
        persist<void*>*    objects {nullptr};   // directory
#ifdef USE_ESLOCO
        void*              mspadd;
#else
        mspace             ms {};
#endif
        uint8_t            padding[1024-32];
    };

    PersistentHeader* per {nullptr};

#ifdef USE_ESLOCO
    EsLoco<persist> esloco {};
#endif

    // Set to true to get a report of leaks on the destructor
    bool enableAllocStatistics = false;

    struct AStats {
        void*    addr;
        uint64_t size;
        AStats(void* addr, uint64_t size) : addr{addr}, size{size} {}
        bool operator < (const AStats& rhs) const {
            return addr < rhs.addr;
        }
    };

    std::set<AStats> statsSet {};
    uint64_t statsAllocBytes {0};
    uint64_t statsAllocNum {0};

    //
    // Private methods
    //

    // Flush touched cache lines
    inline void flush_range(uint8_t* addr, size_t length) {
        const int cache_line_size = 64;
        uint8_t* ptr = addr;
        uint8_t* last = addr + length;
        for (; ptr < last; ptr += cache_line_size) PWB(ptr);
    }

    void copyFromTo(uint8_t* from , uint8_t* to, int fromIdx) {
        uint64_t lcxsize = tl_cx_size;
        tl_cx_size = fromIdx * g_main_size;
        uint64_t usedSize = esloco.getUsedSize();
        if(usedSize==0) usedSize = g_main_size;
        tl_cx_size = lcxsize;
        std::memcpy(to, from, usedSize);
        flush_range(to, usedSize);
    }

    inline void flushDeferredPWBs() {
        for (int k = 0; k < tl_pwb_idx; k++) {
            bool alreadyflushed = false;
            for (int j = 0; j < k; j++) {
                if (tl_pwb_log[k] == tl_pwb_log[j]) {
                    alreadyflushed = true;
                    break;
                }
            }
            if (!alreadyflushed) PWB(tl_pwb_log[k]);
        }
    }


public:

    CX() : dommap{true},maxThreads{MAX_THREADS}{   // TODO: FIX THIS: it should be proportional to MAX_THREADS

        if (dommap) {
            base_addr = (uint8_t*)0x7fddc0000000;
            max_size = PM_REGION_SIZE + 1024;
            // Check if the file already exists or not
            struct stat buf;
            if (stat(MMAP_FILENAME, &buf) == 0) {
                // File exists
                //std::cout << "Re-using memory region\n";
                fd = open(MMAP_FILENAME, O_RDWR|O_CREAT, 0755);
                assert(fd >= 0);
                // mmap() memory range
                uint8_t* got_addr = (uint8_t *)mmap(base_addr, max_size, (PROT_READ | PROT_WRITE), MAP_SHARED_VALIDATE | PM_FLAGS, fd, 0);
                if (got_addr == MAP_FAILED || got_addr != base_addr) {
                    perror("ERROR: mmap() is not working !!! ");
                    printf("got_addr = %p instead of %p\n", got_addr, base_addr);
                    assert(false);
                }
                per = reinterpret_cast<PersistentHeader*>(base_addr);
                if(per->id != MAGIC_ID) createFile();
                g_main_size = (max_size - sizeof(PersistentHeader))/MAX_COMBINEDS;
                g_main_addr = base_addr + sizeof(PersistentHeader);
                g_main_addr_end = g_main_addr + g_main_size;
                g_region_end = g_main_addr +MAX_COMBINEDS*g_main_size;

                combs = new Combined[MAX_COMBINEDS];
                for (int i = 0; i < maxThreads; i++) enqueuers[i].store(nullptr, std::memory_order_relaxed);
                for (int i = 0; i < maxThreads; i++) preRetired[i] = new CircularArray<Node>(hp,i);
                combs[per->curComb.load()].head = sentinel;
                for(int i = 0; i < MAX_COMBINEDS; i++){
                    combs[i].root = g_main_addr + i*g_main_size;
                }
                sentinel->refcnt.store(1, std::memory_order_relaxed);
                Combined* comb = &combs[per->curComb.load()];
                comb->rwLock.setReadLock();
                #ifdef USE_ESLOCO
                        esloco.init(g_main_addr, g_main_size, false);
                #endif
                //recover();
            } else {
                createFile();
            }
        }
    }


    void createFile(){
        // File doesn't exist
        fd = open(MMAP_FILENAME, O_RDWR|O_CREAT, 0755);
        assert(fd >= 0);
        if (lseek(fd, max_size-1, SEEK_SET) == -1) {
            perror("lseek() error");
        }
        if (write(fd, "", 1) == -1) {
            perror("write() error");
        }
        // mmap() memory range
        uint8_t* got_addr = (uint8_t *)mmap(base_addr, max_size, (PROT_READ | PROT_WRITE), MAP_SHARED_VALIDATE | PM_FLAGS, fd, 0);
        if (got_addr == MAP_FAILED || got_addr != base_addr) {
            perror("ERROR: mmap() is not working !!! ");
            printf("got_addr = %p instead of %p\n", got_addr, base_addr);
            assert(false);
        }
        // No data in persistent memory, initialize
        per = new (base_addr) PersistentHeader;
        g_main_size = (max_size - sizeof(PersistentHeader))/MAX_COMBINEDS;
        g_main_addr = base_addr + sizeof(PersistentHeader);
        g_main_addr_end = g_main_addr + g_main_size;
        g_region_end = g_main_addr +MAX_COMBINEDS*g_main_size;
        PWB(&per->curComb);

        combs = new Combined[MAX_COMBINEDS];
        for (int i = 0; i < maxThreads; i++) enqueuers[i].store(nullptr, std::memory_order_relaxed);
        for (int i = 0; i < maxThreads; i++) preRetired[i] = new CircularArray<Node>(hp,i);
        combs[0].head = sentinel;

        for(int i = 0; i < MAX_COMBINEDS; i++){
            combs[i].root = g_main_addr + i*g_main_size;
        }
        sentinel->refcnt.store(1, std::memory_order_relaxed);

        Combined* comb = &combs[per->curComb.load()];
        comb->rwLock.setReadLock();
        updateTx<bool>([&] () {
#ifdef USE_ESLOCO
            esloco.init(g_main_addr, g_main_size, true);
            per->objects = (persist<void*>*)esloco.malloc(sizeof(void*)*NUM_OBJS);
#else
            per->used_size = g_main_size;
            per->ms = create_mspace_with_base(g_main_addr, g_main_size, false);
            per->objects = (persist<void*>*)mspace_malloc(per->ms, sizeof(void*)*NUM_OBJS);
#endif
            for (int i = 0; i < NUM_OBJS; i++) {
                per->objects[i] = nullptr;
            }
            return true;
        });
#ifdef USE_ESLOCO
#else
        per->used_size = (uint8_t*)(&per->used_size) - ((uint8_t*)base_addr+sizeof(PersistentHeader))+128;
#endif
        flush_range((uint8_t*)per,sizeof(PersistentHeader));
        PFENCE();
        per->id = MAGIC_ID;
        PWB(&per->id);
        PSYNC();
    }


    ~CX() {
        for (int i = 0; i < maxThreads; i++) {
            delete preRetired[i];
        }
        delete[] combs;
        delete sentinel;

        // Must do munmap() if we did mmap()
        if (dommap) {
            //destroy_mspace(ms);
            munmap(base_addr, max_size);
            close(fd);
        }
        if (enableAllocStatistics) { // Show leak report
            std::cout << "CX: statsAllocBytes = " << statsAllocBytes << "\n";
            std::cout << "CX: statsAllocNum = " << statsAllocNum << "\n";
        }
    }

    static std::string className() { return "CXPTM"; }

    template <typename T>
    static inline T* get_object(int idx) {
        return reinterpret_cast<T*>( gCX.per->objects[idx].pload() );
    }

    template <typename T>
    static inline void put_object(int idx, T* obj) {
        gCX.per->objects[idx].pstore(obj);
    }


    // Helper function: Create node, protect it, and enqueue it in the Turn Queue
    template<typename R, class F>
    inline Node* createAndEnqueueNode(F&& func, const int tid) {
        std::function<R()> stackf {func};
        Node* myNode = new Node(*(std::function<uint64_t()>*)&stackf, tid);
        hp.protectPtrRelease(kHpMyNode, myNode, tid);
        enqueue(myNode, tid);
        return myNode;
    }


    template<typename R,class F>
    R ns_read_transaction(F&& func) {
        if (tl_nested_read_trans > 0) return (R)func();
        int tid = ThreadRegistry::getTID();
        ++tl_nested_read_trans;
        Node* myNode = nullptr;
        for (int i=0; i < MAX_READ_TRIES + maxThreads; i++) {
            const int curCombIndex = per->curComb.load();
            Combined* lcomb = &combs[curCombIndex];
            // Enqueue read-only operation as if it was a mutation
            if (i == MAX_READ_TRIES) myNode = createAndEnqueueNode<R,F>(func, tid);
            PWB(&per->curComb);
            // This PFENCE is ommited because ordering is guaranteed by the MFENCE on the ri.arrive()
            //PFENCE();
            if (lcomb->rwLock.sharedTryLock(tid)) {
                if (curCombIndex == per->curComb.load()) {
                    tl_cx_size = curCombIndex*g_main_size;
                    auto ret = func();
                    lcomb->rwLock.sharedUnlock(tid);
                    --tl_nested_read_trans;
                    return (R)ret;
                }
                lcomb->rwLock.sharedUnlock(tid);
            }
        }
        --tl_nested_read_trans;
        PSYNC();
        return (R)myNode->result.load();
    }


    // Non-static thread-safe read-write transaction
    template<typename R,class F>
    R ns_write_transaction(F&& func) {
        if (tl_nested_write_trans > 0) return (R)func();
        const int tid = ThreadRegistry::getTID();
        Node* const myNode = createAndEnqueueNode<R,F>(func, tid);
        const uint64_t myTicket = myNode->ticket.load();
        // Get one of the Combined instances on which to apply mutation(s)
        Combined* newComb = nullptr;
        int newCombIndex = 0;
        for (int i = 0; i < MAX_COMBINEDS; i++) {
            if (combs[i].rwLock.exclusiveTryLock(tid)) {
                newComb = &combs[i];
                newCombIndex = i;
                break;
            }
        } // RWLocks: newComb=X
        if (newComb == nullptr) {
            std::cout << "ERROR: not enough Combined instances\n";
            assert(false);
        }

        Node* mn = newComb->head;
        if (mn != nullptr && mn->ticket.load() >= myTicket) {
            newComb->rwLock.exclusiveUnlock();
            PWB(&per->curComb);
            PSYNC();
            return (R)myNode->result.load();
        }

        tl_cx_size = newCombIndex*g_main_size;
        tl_pwb_idx = 0;
        ++tl_nested_write_trans;
        // Apply all mutations starting from 'head' up to our node or the end of the list
        Combined* lcomb = nullptr;
        int64_t lCombIndex = -1;
        while (mn != myNode) {
            if (mn == nullptr || mn == mn->next.load()) {
                if (lcomb != nullptr || (lCombIndex = getCombined(myTicket,tid)) == -1) {
                    if (mn != nullptr) newComb->updateHead(mn);
                    flushDeferredPWBs();
                    newComb->rwLock.exclusiveUnlock();
                    PWB(&per->curComb);
                    PSYNC();
                    --tl_nested_write_trans;
                    tl_cx_size = 0;
                    return (R)myNode->result.load();
                }
                lcomb = &combs[lCombIndex];

                // Neither the 'instance' nor the 'head' will change now that we hold the shared lock
                copyFromTo(lcomb->root, newComb->root, lCombIndex);
                mn = lcomb->head;
                newComb->updateHead(mn);
                lcomb->rwLock.sharedUnlock(tid);
                continue;
            }
            Node* lnext = hp.protectPtr(kHpHead, mn->next.load(), tid);
            if (mn == mn->next.load()) continue;
            // Execute the mutation for this node and save the result
            lnext->result.store(lnext->mutation(), std::memory_order_relaxed);
            hp.protectPtrRelease(kHpNext, lnext, tid);
            mn = lnext;
        }
        newComb->updateHead(mn);
        newComb->rwLock.downgrade();  // RWLocks: newComb=H

        // Execute deferred PWB()s of the modifications
        flushDeferredPWBs();
        // This PFENCE is ommited because it will be executed per->curComb.compare_exchange_strong(tmp, newCombIndex) that
        // will issue the necessary fence that orders previous PWB and the store on curComb.
        //PFENCE();

        // Make the mutation visible to other threads by advancing curComb
        for (int i = 0; i < maxThreads; i++) {
            int lcombIndex = per->curComb.load();
            Combined* const lcomb = &combs[lcombIndex];
            if (!lcomb->rwLock.sharedTryLock(tid)) continue;
            if (lcomb->head == nullptr) {
                lcomb->rwLock.sharedUnlock(tid);
                continue;
            }
            if (lcomb->head->ticket.load() >= myTicket) {
                lcomb->rwLock.sharedUnlock(tid);
                if (lcombIndex != per->curComb.load()) continue;
                break;
            }
            if (per->curComb.compare_exchange_strong(lcombIndex, newCombIndex)){
                PWB(&per->curComb);
                PSYNC();
                lcomb->rwLock.setReadUnlock();
                --tl_nested_write_trans;
                tl_cx_size = 0;
                // Retire nodes from lComb->head to newComb->head
                Node* node = lcomb->head;
                lcomb->rwLock.sharedUnlock(tid);
                while (node != mn) {
                    Node* lnext = node->next.load();
                    preRetired[tid]->add(node);
                    node = lnext;
                }
                return (R)myNode->result.load();
            }
            lcomb->rwLock.sharedUnlock(tid);
        }
        newComb->rwLock.setReadUnlock();
        PWB(&per->curComb);
        PSYNC();
        --tl_nested_write_trans;
        tl_cx_size = 0;
        return (R)myNode->result.load();
    }


    /**
     * Enqueue algorithm from the Turn queue, adding a monotonically incrementing ticket
     * Steps when uncontended:
     * 1. Add node to enqueuers[]
     * 2. Insert node in tail.next using a CAS
     * 3. Advance tail to tail.next
     * 4. Remove node from enqueuers[]
     */
    void enqueue(Node* myNode, const int tid) {
        enqueuers[tid].store(myNode);
        const int numThreads = ThreadRegistry::getMaxThreads();
        for (int i = 0; i < numThreads; i++) {
            if (enqueuers[tid].load() == nullptr) {
                //hp.clear(tid);
                return; // Some thread did all the steps
            }
            Node* ltail = hp.protectPtr(kHpTail, tail.load(), tid);
            if (ltail != tail.load()) continue; // If the tail advanced numThreads times, then my node has been enqueued
            if (enqueuers[ltail->enqTid].load() == ltail) {  // Help a thread do step 4
                Node* tmp = ltail;
                enqueuers[ltail->enqTid].compare_exchange_strong(tmp, nullptr);
            }
            for (int j = 1; j < numThreads+1; j++) {         // Help a thread do step 2
                Node* nodeToHelp = enqueuers[(j + ltail->enqTid) % numThreads].load();
                if (nodeToHelp == nullptr) continue;
                Node* nodenull = nullptr;
                ltail->next.compare_exchange_strong(nodenull, nodeToHelp);
                break;
            }
            Node* lnext = ltail->next.load();
            if (lnext != nullptr) {
                hp.protectPtr(kHpTailNext, lnext, tid);
                if (ltail != tail.load()) continue;
                lnext->ticket.store(ltail->ticket.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
                tail.compare_exchange_strong(ltail, lnext); // Help a thread do step 3:
            }
        }
        enqueuers[tid].store(nullptr, std::memory_order_release); // Do step 4, just in case it's not done
    }


    /*
     * Mean to be called from user code when something bad happens and the whole
     * transaction needs to be aborted.
     * This function has strict semantics.
     */
    inline void abort_transaction(void);


    template <typename T, typename... Args> static T* tmNew(Args&&... args) {
        CX& r = gCX;
#ifdef USE_ESLOCO
        void* addr = r.esloco.malloc(sizeof(T));
        assert(addr != nullptr);
#else
        void* addr = mspace_malloc( ((uint8_t*)(r.per->ms))+tl_cx_size, sizeof(T));
        assert(addr != 0);
#endif
        T* ptr = new (addr) T(std::forward<Args>(args)...); // placement new
        if (r.enableAllocStatistics) {
            r.statsAllocBytes += sizeof(T);
            r.statsAllocNum++;
            r.statsSet.insert({addr, sizeof(T)});
        }
        return ptr;
    }


    /*
     * De-allocator
     * Calls destructor of T and then de-allocate the memory using the internal allocator (Doug Lea or EsLoco)
     */
    template<typename T> static void tmDelete(T* obj) {
        if (obj == nullptr) return;
        obj->~T();
        CX& r = gCX;
#ifdef USE_ESLOCO
        r.esloco.free(obj);
#else
        mspace_free( ((uint8_t*)(r.per->ms))+tl_cx_size,obj);
#endif
        if (r.enableAllocStatistics) {
            auto search = r.statsSet.find({obj,0});
            if (search == r.statsSet.end()) {
                std::cout << "Attemped free() of unknown address\n";
                assert(false);
                return;
            }
            r.statsAllocBytes -= search->size;
            r.statsAllocNum--;
            r.statsSet.erase(*search);
        }
    }

    /* Allocator for C methods (like memcached) */
    static void* pmalloc(size_t size) {
        CX& r = gCX;
#ifdef USE_ESLOCO
        void* addr = r.esloco.malloc(size);
        assert(addr != nullptr);
#else
        void* addr = mspace_malloc( ((uint8_t*)(r.per->ms))+tl_cx_size , size);
        assert(addr != 0);
#endif
        assert (addr != 0);
        if (r.enableAllocStatistics) {
            r.statsAllocBytes += size;
            r.statsAllocNum++;
            r.statsSet.insert({addr, size});
        }
        return addr;
    }

    /* De-allocator for C methods (like memcached) */
    static void pfree(void* ptr) {
        CX& r = gCX;
#ifdef USE_ESLOCO
        r.esloco.free(ptr);
#else
        mspace_free( ((uint8_t*)(r.per->ms))+tl_cx_size, ptr);
#endif
        if (r.enableAllocStatistics) {
            auto search = r.statsSet.find({ptr,0});
            if (search == r.statsSet.end()) {
                std::cout << "Attemped pfree() of unknown address\n";
                assert(false);
                return;
            }
            r.statsAllocBytes -= search->size;
            r.statsAllocNum--;
            r.statsSet.erase(*search);
        }
    }

    template<typename R,class F> inline static R readTx(F&& func) {
        return gCX.ns_read_transaction<R>(func);
    }

    template<typename R,class F> inline static R updateTx(F&& func) {
        return gCX.ns_write_transaction<R>(func);
    }

    // Doesn't actually do any checking. That functionality exists only for RomulusLog and RomulusLR
    static bool consistency_check(void) {
        return true;
    }
};
}
#endif   // _CX_PERSISTENT_TRANSACTIONAL_MEMORY_H_
