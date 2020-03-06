/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _ROMULUS_LR_H_
#define _ROMULUS_LR_H_

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
#include <vector>
#include <functional>
#include <thread>

#include "../../common/pfences.h"
#include "common/ThreadRegistry.hpp"

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
#define PM_FILE_NAME   "/dev/shm/romuluslr_shared"
#endif


/* <h1> Romulus using Left-Right plus flat-combining </h1>
 *
 * Romulus using Left-Right plus flat-combining.
 * It provides wait-free (population oblivious) progress for readers and blocking (starvationg-free) for writers.
 *
 * Because we wanted the user to just include this header, we have put everything in the header,
 * which isn't pretty, but it makes life easier for application developers.
 *
 * Left-Right paper: https://github.com/pramalhe/ConcurrencyFreaks/blob/master/papers/left-right-2014.pdf
 * Flat Combining paper:  http://dl.acm.org/citation.cfm?id=1810540
 * A post about Left-Right with Flat Combining: http://concurrencyfreaks.com/2017/07/left-right-and-c-rw-wp-with-flat.html
 *
 * We have the following classes in this file:
 * RIStaticPerThread -> A ReadIndicator to be used in the URCU inside Left-Right
 * RomulusLR         -> Romulus using Left-Right and Flat-Combining for Writers
 * persist<T>        -> Annotation for persistent types T
 *
 * How to use RomulusLR:
 * persist<UserObject> obj;
 * gLRTM.write_transaction([&obj] () {obj.mutative_method()} );
 * gLRTM.read_transaction([&obj] () {obj.non_mutative_method()} );
 *
 * DO NOT modify global variables or thread-locals inside a write_transaction()
 * because they will be modified twice.
 * DO NOT pass by reference stuff to the lambda and then modify it inside the
 * lambda because the lambda is executed twice.
 */
namespace romuluslr {

extern uint64_t g_main_size;
extern uint8_t* g_main_addr;
extern uint8_t* g_main_addr_end;
extern bool g_right;

class RIStaticPerThread {
private:
    static const uint64_t NOT_READING = 0;
    static const uint64_t READING = 1;
    static const int CLPAD = 128/sizeof(uint64_t);

    const int maxThreads;
    alignas(128) std::atomic<uint64_t>* states;

public:
    RIStaticPerThread(int maxThreads) : maxThreads{maxThreads} {
        states = new std::atomic<uint64_t>[maxThreads*CLPAD];
        for (int tid = 0; tid < maxThreads; tid++) {
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
        for (int tid = 0; tid < ThreadRegistry::getMaxThreads(); tid++) {
            if (states[tid*CLPAD].load() != NOT_READING) return false;
        }
        return true;
    }
};


// Forward declaration for global instance of RomulusLR
class RomulusLR;
extern RomulusLR gRomLR;
// Counter of nested write transactions
extern thread_local int64_t tl_nested_write_trans;
// Counter of nested read-only transactions
extern thread_local int64_t tl_nested_read_trans;


// Possible values for 'leftRight' and tl_lrromulus variable
static const int TRAVERSE_LEFT = 0;
static const int TRAVERSE_RIGHT = 1;
// This variable indicates which of the instances to be used by the ongoing transaction
extern thread_local int tl_lrromulus;

// Doug Lea's allocator declarations
typedef void* mspace;
extern void* mspace_malloc(mspace msp, size_t bytes);
extern void mspace_free(mspace msp, void* mem);



class RomulusLR {
    // Id for sanity check of Romulus
    static const uint64_t MAGIC_ID = 0x1337BAB5;

    // Possible values for "state"
    static const int IDLE = 0;
    static const int MUTATING = 1;
    static const int COPYING = 2;

    // Number of log entries in a chunk of the log
    static const int CHUNK_SIZE = 1024;

    // Filename for the mapping file
    const char* MMAP_FILENAME = PM_FILE_NAME;

    // Member variables
    bool dommap;
    int fd = -1;
    uint8_t* base_addr;
    uint64_t max_size;
    uint8_t* main_addr;
    uint8_t* back_addr;

    // Each log entry is two words (8+8 = 16 bytes)
    struct LogEntry {
        size_t    offset;  // Pointer offset in bytes, relative to main_addr
        uint64_t  length;  // Range length of data at pointer offset
    };

    struct LogChunk {
        LogEntry  entries[CHUNK_SIZE];
        uint64_t  num_entries { 0 };
        LogChunk* next        { nullptr };
    };

    // There is always at least one (empty) chunk in the log, it's the head
    LogChunk* log_head = new LogChunk;
    LogChunk* log_tail = log_head;

    // One instance of this is at the start of base_addr, in persistent memory
    struct PersistentHeader {
        uint64_t         id {0};          // Validates intialization
        std::atomic<int> state {IDLE}; // Current state of consistency
        void**           objects {};      // Objects directory
        mspace           ms {};           // Pointer to allocator's metadata
        uint64_t         used_size {0};   // It has to be the last, to calculate the used_size
    };

    PersistentHeader* per {nullptr};      // Volatile pointer to start of persistent memory
    uint64_t log_size = 0;
    bool logEnabled = true;

private:
    static const int CLPAD = 128/sizeof(uintptr_t);
    static const int MAX_THREADS = 128;
    static const int LOCKED = 1;
    static const int UNLOCKED = 0;
    const int maxThreads;
    // Stuff use by the Flat Combining mechanism
    alignas(128) std::atomic< std::function<void()>* >* fc; // array of atomic pointers to functions
    // Stuff used by the Left-Right mechanism
    alignas(128) std::atomic<int> writersMutex { UNLOCKED };
    alignas(128) std::atomic<int> leftRight { TRAVERSE_LEFT };
    alignas(128) std::atomic<int> versionIndex { 0 };
    RIStaticPerThread ri[2] { MAX_THREADS, MAX_THREADS };


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
    void copyMainToBack();
    void copyBackToMain();
    void createFile();

    /*
     * Called at the end of a transaction to replicate the mutations on "back",
     * or when abort_transaction() is called by the user, to rollback the
     * mutations on "main".
     * Deletes the log as it is being applied.
     */
    inline void apply_pwb(uint8_t* from_addr) {
        // Apply the log to the instance on 'to_addr', copying data from the instance at 'from_addr'
        LogChunk* chunk = log_head;
        while (chunk != nullptr) {
            for (int i = 0; i < chunk->num_entries; i++) {
                LogEntry& e = chunk->entries[i];
                //std::memcpy(to_addr + e.offset, from_addr + e.offset, e.length);
                flush_range(from_addr + e.offset, e.length);
            }
            chunk = chunk->next;
        }
    }

    /*
     * Called at the end of a transaction to replicate the mutations on "back",
     * or when abort_transaction() is called by the user, to rollback the
     * mutations on "main".
     * Deletes the log as it is being applied.
     */
    inline void apply_log(uint8_t* from_addr, uint8_t* to_addr) {
        // Apply the log to the instance on 'to_addr', copying data from the instance at 'from_addr'
        LogChunk* chunk = log_head;
        while (chunk != nullptr) {
            for (int i = 0; i < chunk->num_entries; i++) {
                LogEntry& e = chunk->entries[i];
                std::memcpy(to_addr + e.offset, from_addr + e.offset, e.length);
                //flush_range(to_addr + e.offset, e.length);
            }
            //LogChunk* next = chunk->next;
            //if (chunk != log_head) delete chunk;
            chunk = chunk->next;
        }
        // Clear the log, leaving one chunk for next transaction, with zero'ed entries
        /*
        log_tail = log_head;
        log_head->num_entries = 0;
        log_head->next = nullptr;*/
    }

    inline void clear_log() {
        LogChunk* chunk = log_head->next;
        while (chunk != nullptr) {
            LogChunk* next = chunk->next;
            delete chunk;
            chunk = next;
        }
        // Clear the log, leaving one chunk for next transaction, with zero'ed entries
        log_tail = log_head;
        log_head->num_entries = 0;
        log_head->next = nullptr;
    }

    inline void toggleVersionAndWait() {
        const int localVI = versionIndex.load();
        const int prevVI = localVI & 0x1;
        const int nextVI = (localVI+1) & 0x1;
        // Wait for Readers from next version
        while (!ri[nextVI].isEmpty()) {} // spin
        // Toggle the versionIndex variable
        versionIndex.store(nextVI);
        // Wait for Readers from previous version
        while (!ri[prevVI].isEmpty()) {} // spin
    }


public:
    /*
     * Adds to the log the current contents of the memory location starting at
     * 'addr' with a certain 'length' in bytes
     */
    inline void add_to_log(void* addr, int length) noexcept {

        if (!logEnabled) return;
        // If the log has more than 1/4 of the entire size then skip the log
        // and copy the used size of the main region.
        if (log_size > per->used_size/4) {
            logEnabled = false;
            return;
        }
        size_t addrCL = ((size_t)addr)>>6;
        // Get the current chunk of log and if it is already full then create a new chunk and add the entry there.
        LogChunk* chunk = log_tail;
        bool sameCL = false;

        if(addrCL == (size_t)((uint8_t*)addr+length)>>6){
        	sameCL = true;
        	int size = chunk->num_entries;
        	for(int i=size-1;i>=0 && i>size-16;i--){
				LogEntry& e1 = chunk->entries[i];

				size_t offCL = (size_t)(e1.offset+main_addr)>>6;
				if(e1.length==64 && (size_t)(offCL<<6) == (size_t)(e1.offset+main_addr)){
					if(offCL == addrCL) return;
				}
        	}
        }
        if (chunk->num_entries == CHUNK_SIZE) {
            chunk = new LogChunk();
            log_tail->next = chunk;
            log_tail = chunk;
        }
        LogEntry& e = chunk->entries[chunk->num_entries];

        if(sameCL){
        	size_t cl =addrCL<<6;
            e.offset = (uint8_t*)cl - main_addr;
        	e.length = 64;
        }else {
        	e.offset = (uint8_t*)addr - main_addr;
        	e.length = length;
        }
        log_size+=length;
        chunk->num_entries++;
    }

    RomulusLR();
    ~RomulusLR();

    static std::string className() { return "RomulusLR"; }

    // The allocator metada will rollback but this store will persist which is _wrong_
    template <typename T>
    static inline T* get_object(int idx) {
        if (tl_lrromulus == TRAVERSE_LEFT) {
            return static_cast<T*>(gRomLR.per->objects[idx]);
        } else {
            return reinterpret_cast<T*>( *(size_t*)((uint8_t*)&(gRomLR.per->objects[idx]) + g_main_size) );
        }
    }

    // The allocator metada will rollback but this store will persist which is _wrong_
    template <typename T>
    static inline void put_object(int idx, T* obj) {
        gRomLR.per->objects[idx] = obj;
        gRomLR.add_to_log(&(gRomLR.per->objects[idx]), sizeof(void*));
        PWB(&(gRomLR.per->objects[idx]));
    }


    bool compareMainAndBack();
    /*
     * Must be called at the beginning of each (write) transaction.
     */
    inline void begin_transaction() {
        // Check for nested transaction
    	tl_nested_write_trans++;
        if (tl_nested_write_trans >1) return;

        per->state.store(MUTATING, std::memory_order_relaxed);
        PWB(&per->state);
        // One PFENCE() is enough for all user modifications because no ordering is needed between them.
        PFENCE();
    }


    /*
     * Must be called at the end of each (write) transaction.
     */
    inline void end_transaction() {
        // Check for nested transaction
    	--tl_nested_write_trans;
        if (tl_nested_write_trans >0) return;
        // Do a PFENCE() to make persistent the stores done in 'main' and on the Romulus persistent
        // data (due to memory allocation).
        // We only care about ordering here, not about durability, therefore, no need to block.
        PFENCE();
        per->state.store(COPYING, std::memory_order_relaxed);        /* str_rel */
        PWB(&per->state);
        PWB(&per->used_size);
        // PSYNC() here to have ACID Durability on the mutations done to "main" and make the change of state visible
        PSYNC();
        // Apply log, copying data from 'main' to 'back'
        if (logEnabled) {
            apply_log(main_addr, back_addr);
        } else {
            copyMainToBack();
            clear_log();
            logEnabled = true;
        }
        log_size = 0;
        PFENCE();
        per->state.store(IDLE, std::memory_order_relaxed);
    }



    /*
     * Recovers from an incomplete transaction if needed
     */
    inline void recover();


    /*
     * Same as begin/end transaction, but with a lambda.
     * Calling abort_transaction() from within the lambda is not allowed.
     */
    template<typename R, class F>
    R transaction(F&& func) {
        begin_transaction();
        R retval = func();
        end_transaction();
        return retval;
    }

    template<class F>
    static void transaction(F&& func) {
        gRomLR.begin_transaction();
        func();
        gRomLR.end_transaction();
    }


    /*
     * Non static, thread-safe
     * Progress: Blocking (starvation-free)
     */
    template<typename Func>
    void ns_write_transaction(Func&& mutativeFunc) {
        if (tl_nested_write_trans > 0) {
            mutativeFunc();
            return;
        }
        std::function<void()> myfunc = mutativeFunc;
        int tid = ThreadRegistry::getTID();
        // Add our mutation to the array of flat combining
        fc[tid*CLPAD].store(&myfunc, std::memory_order_release);

        // Lock writersMutex
        while (true) {
            int unlocked = UNLOCKED;
            if (writersMutex.load() == UNLOCKED &&
                writersMutex.compare_exchange_strong(unlocked, LOCKED)) break;
            // Check if another thread executed my mutation
            if (fc[tid*CLPAD].load(std::memory_order_acquire) == nullptr) return;
            std::this_thread::yield();
        }

        bool somethingToDo = false;
        const int maxTid = ThreadRegistry::getMaxThreads();
        // Save a local copy of the flat combining array
        std::function<void()>* lfc[maxTid];
        for (int i = 0; i < maxTid; i++) {
            lfc[i] = fc[i*CLPAD].load(std::memory_order_acquire);
            if (lfc[i] != nullptr) somethingToDo = true;
        }
        // Check if there is at least one operation to apply
        if (!somethingToDo) {
            writersMutex.store(UNLOCKED, std::memory_order_release);
            return;
        }
        ++tl_nested_write_trans;
        per->state.store(MUTATING, std::memory_order_relaxed);
        PWB(&per->state);
        // One PFENCE() is enough for all user modifications because no ordering is needed between them.
        PFENCE();
        g_right = true;
        // Readers can only see the changes after making sure they are persisted
        leftRight.store(TRAVERSE_RIGHT);
        tl_lrromulus = TRAVERSE_LEFT;
        toggleVersionAndWait();  // This is a synchronize_rcu()
        // Apply all mutativeFunc
        for (int i = 0; i < maxTid; i++) {
            if (lfc[i] == nullptr) continue;
            (*lfc[i])();
        }
        apply_pwb(main_addr);
        PFENCE();
        per->state.store(COPYING, std::memory_order_relaxed);
        PWB(&per->state);
        // PSYNC() here to have ACID Durability on the mutations done to 'main' and make the change of state visible
        PSYNC();
        // Readers can only see the changes after making sure they are persisted
        leftRight.store(TRAVERSE_LEFT);
        toggleVersionAndWait();  // This is a synchronize_rcu()
        g_right = false;
        // After changing state to COPYING all applied mutativeFunc are visible and persisted
        for (int i = 0; i < maxTid; i++) {
            if (lfc[i] == nullptr) continue;
            fc[i*CLPAD].store(nullptr, std::memory_order_release);
        }
        // Apply log, copying data from 'main' to 'back'
        if (logEnabled) {
            apply_log(main_addr, back_addr);
            apply_pwb(back_addr);
        } else {
            copyMainToBack();

            logEnabled = true;
        }
        clear_log();
        log_size = 0;

        PFENCE();
        per->state.store(IDLE, std::memory_order_relaxed);
        // unlock()
        writersMutex.store(UNLOCKED, std::memory_order_release);
        --tl_nested_write_trans;
        //consistency_check();
    }


    /*
     * Non-static
     * Progress: Wait-Free Population Oblivious
     */
    template<typename Func>
    void ns_read_transaction(Func&& readFunc) {
        if (tl_nested_read_trans > 0) {
            readFunc();
            return;
        }
        int tid = ThreadRegistry::getTID();
        const int localVI = versionIndex.load();
        ++tl_nested_read_trans;
        ri[localVI].arrive(tid);  // This is an rcu_read_lock()
        int lr = leftRight.load();
        if(lr!=tl_lrromulus) tl_lrromulus = lr;

        readFunc();
        ri[localVI].depart(tid);  // This is an rcu_read_unlock()
        --tl_nested_read_trans;
    }


    template <typename T, typename... Args>
    static T* tmNew(Args&&... args) {
        const RomulusLR& r = gRomLR;
        void* addr = mspace_malloc(r.per->ms, sizeof(T));
        assert(addr != 0);
        T* ptr = new (addr) T(std::forward<Args>(args)...); // placement new
        if (r.per->used_size < (uint8_t*)addr - r.main_addr + sizeof(T) + 128) {
            r.per->used_size = (uint8_t*)addr - r.main_addr + sizeof(T) + 128;
            PWB(&r.per->used_size);
        }
        return ptr;
    }

    /*
     * De-allocator
     * Calls destructor of T and then reclaims the memory using Doug Lea's free
     */
    template<typename T>
    static void tmDelete(T* obj) {
        if (obj == nullptr) return;
        obj->~T();
        mspace_free(gRomLR.per->ms,obj);
    }

    /* Allocator for C methods (like memcached) */
    static void* pmalloc(size_t size) {
        const RomulusLR& r = gRomLR;
        void* addr = mspace_malloc(r.per->ms, size);
        assert (addr != 0);
        if (r.per->used_size < (uint8_t*)addr - r.main_addr + size + 128) {
            r.per->used_size = (uint8_t*)addr - r.main_addr + size + 128;
            PWB(&r.per->used_size);
        }
        return addr;
    }

    /* De-allocator for C methods (like memcached) */
    static void pfree(void* ptr) {
        return mspace_free(gRomLR.per->ms, ptr);
    }

    template<class F>
    inline static void readTx(F&& func) {
        gRomLR.ns_read_transaction(func);
    }

    template<class F>
    inline static void updateTx(F&& func) {
        gRomLR.ns_write_transaction(func);
    }


    // TODO: Remove these two once we make CX have void transactions
    template<typename R,class F>
    inline static R readTx(F&& func) {
        gRomLR.ns_read_transaction([&]() {func();});
        return R{};
    }
    template<typename R,class F>
    inline static R updateTx(F&& func) {
        gRomLR.ns_write_transaction([&]() {func();});
        return R{};
    }

    /*
     * Thread-safe. Compares the contents of 'main' and 'back'.
     * This method MUST be called outside a transaction.
     */
    static bool consistency_check(void) {
        if (tl_nested_write_trans > 0) {
            printf("Warning: don't call consistency_check() inside a transaction\n");
        } else {
            while (true) {
                int unlocked = UNLOCKED;
                if (gRomLR.writersMutex.load() == UNLOCKED &&
                    gRomLR.writersMutex.compare_exchange_strong(unlocked, LOCKED)) break;
                std::this_thread::yield();
            }
            gRomLR.compareMainAndBack();
            gRomLR.writersMutex.store(UNLOCKED, std::memory_order_release);
        }
        return true;
    }
};


/*
 * Definition of persist<> type
 * In RomulusLR we interpose the loads and the stores
 */
template<typename T>
struct persist {
    // Stores the actual value (left instance)
    T val {};

    persist() { }

    persist(T initVal) {
        pstore(initVal);
    }

    // Casting operator
    operator T() {
        return pload();
    }

    // Casting to const
    operator T() const {
        return pload();
    }

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

    // Operator &. See pload()
    T* operator&() {

       	if(!g_right){
			return &val;
       	}
        if (tl_lrromulus == TRAVERSE_LEFT) return &val;
        const uint8_t* valaddr = (uint8_t*)&val;
        if(valaddr > g_main_addr && valaddr < g_main_addr_end) return reinterpret_cast<T*>( (uint8_t*)&val + g_main_size );
        return &val;
    }

    // Copy constructor
    persist<T>(const persist<T>& other) {
        pstore(other.pload());
    }

    // Assignment operator from another persist<> instance
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

    // Only modifies the left instance
    inline void pstore(T newVal) {
        val = newVal;
        const uint8_t* valaddr = (uint8_t*)&val;
        if (valaddr >= g_main_addr && valaddr < g_main_addr_end) {
            //PWB(&val);
            gRomLR.add_to_log(&val,sizeof(T));
        }
    }

    // Muuuuahahahah!
    // This is the most evil method in the entire repository
    inline T pload() const {
    	if (!g_right || tl_lrromulus == TRAVERSE_LEFT) return val;
        const uint8_t* valaddr = (uint8_t*)&val;
        if (valaddr > g_main_addr && valaddr < g_main_addr_end) return *reinterpret_cast<T*>( (uint8_t*)&val + g_main_size );
        return val;
    }
};


} // end of namespace lrtm

#endif /* _ROMULUS_LR_H_ */
