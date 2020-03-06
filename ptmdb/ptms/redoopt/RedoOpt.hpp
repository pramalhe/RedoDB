/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _REDOOPT_PERSISTENCY_H_
#define _REDOOPT_PERSISTENCY_H_

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
#include <type_traits>
#include <chrono>
#include <iostream>
#include <fstream>


#include "../../common/pfences.h"
#include "../../common/ThreadRegistry.hpp"
#include "../../common/StrongTryRIRWLock.hpp"
#include "../redoopt/HazardPointers.hpp"

using namespace std;
using namespace chrono;

// TODO: take this out if we decide to go with Doug Lea's malloc for CX
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
#define PM_FILE_NAME   "/dev/shm/redoopt_shared"
#endif


namespace redoopt {


/*
 * <h1> CX Persistent Transactional Memory </h1>
 * TODO: explain this...
 *
 *
 *
 */

// Forward declaration of CX to create a global instance
class RedoOpt;
extern RedoOpt gRedo;

// Global with the 'main' size. Used by pload()
extern uint64_t g_main_size;
extern uint8_t* g_main_addr;
extern uint8_t* g_main_addr_end;
extern uint8_t* g_region_end;

// Counter of nested write transactions
extern thread_local int64_t tl_nested_write_trans;
// Counter of nested read-only transactions
extern thread_local int64_t tl_nested_read_trans;

#ifdef MEASURE_PWB
// Used only if MEASURE_PWB is defined
extern thread_local uint64_t tl_num_pwbs;
extern thread_local uint64_t tl_num_pfences;
#endif


// Debug this in gdb with:
// p lines[tid][(iline[tid]-0)%MAX_LINES]
// p lines[tid][(iline[tid]-1)%MAX_LINES]
// p lines[tid][(iline[tid]-2)%MAX_LINES]
#define DEBUG_LINE() lines[tid][iline[tid]%MAX_LINES] = __LINE__; iline[tid]++;

#define ADDR_IS_IN_MAIN(addr) ((uint8_t*)(addr) >= g_main_addr && (uint8_t*)(addr) < g_main_addr_end)
#define ADDR_IS_IN_REGION(addr) ((addr) >= g_main_addr && (addr) < g_region_end)

#ifdef USE_ESLOCO
#include "../../common/EsLoco.hpp"
#else // Use Doug Lea's malloc
// This macro is used in Doug Lea's malloc to adjust weird castings that don't call the '&' operator
#define CX_ADJUSTED_ADDR(addr) (ADDR_IS_IN_REGION(addr)) ? (malloc_chunk*)(addr - tlocal.tl_cx_size) : (malloc_chunk*)addr

typedef void* mspace;
extern void* mspace_malloc(mspace msp, size_t bytes);
extern void mspace_free(mspace msp, void* mem);
extern mspace create_mspace_with_base(void* base, size_t capacity, int locked);
#endif


// Returns the cache line of the address (this is for x86 only)
#define ADDR2CL(_addr) (uint8_t*)((size_t)(_addr) & (~63ULL))

struct varLocal {
	void* st{};
	uint64_t tl_cx_size{0};
	int64_t tl_nested_write_trans{0};
	int64_t tl_nested_read_trans{0};
	bool copy{false};
	uint64_t* writes {nullptr};
	varLocal(){
	    writes = new uint64_t[REGISTRY_MAX_THREADS];
	    for (int i = 0; i < REGISTRY_MAX_THREADS; i++) writes[i]=0;
	}
#ifdef MEASURE_PWB
	// this is actually to measure writes, but it's ok
	~varLocal() {
	    printf("tl_num_pwbs = %ld\n", tl_num_pwbs);
	    /*
	    ofstream dataFile;
	    dataFile.open("write_"+std::to_string((uint64_t)this));
        for (int i = 0; i < REGISTRY_MAX_THREADS; i++) {
            dataFile << i << " " << writes[i] << "\n";
        }
        dataFile.close();
        delete[] writes;
        */
	}
#endif
};

extern thread_local varLocal tlocal;

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
            return reinterpret_cast<T*>( valaddr - tlocal.tl_cx_size );
        }
        return &val;
    }

    inline void pstore(T newVal);

    inline T pload() const {
        uint8_t* valaddr = (uint8_t*)&val;
        const uint64_t cx_size = tlocal.tl_cx_size;
        if (cx_size != 0 && ADDR_IS_IN_MAIN(valaddr)) {
            return *reinterpret_cast<T*>( valaddr + cx_size );
        } else {
            return val;
        }
    }
};


typedef uint64_t SeqTidIdx;

class RedoOpt {
	int NUM_CORES = 0;
	const int MAX_COMBS = 2;
    static const int MAX_READ_TRIES = 10; // Maximum number of times a reader will fail to acquire the shared lock before adding its operation as a mutation
    static const int MAX_THREADS = 41;
    static const int MAX_COMBINEDS = MAX_THREADS+1;
    static const int NUM_OBJS = 8;
    static const int MAXLOGSIZE = 64;
    static const int RINGSIZE = 16192;
    static const int STATESSIZE = 256;
    // Constants for SeqTidIdx. They must sum to 64 bits
    static const int SEQ_BITS = 40;
    static const int TID_BITS = 8;   // Can't have more than 256 threads (should be enough for now)
    static const int IDX_BITS = 16;  // WARNING: ring capacity up to 4098
    const int maxThreads;
    static const uint64_t HASH_BUCKETS = 64;


    // A single entry in the write-set
    struct WriteSetEntry {
        uint8_t*       addr {nullptr};      // Address of value+sequence to change
        uint64_t       oldval {0};          // Previous value
        uint64_t       val {0};             // Desired value to change to
        WriteSetEntry*        next {nullptr};
    };

    // A single entry in the write-set
    struct WriteSetNode {
        WriteSetEntry*     buckets[MAXLOGSIZE];
        WriteSetEntry      log[MAXLOGSIZE];     // Redo log of stores
        WriteSetNode*      next {nullptr};
        WriteSetNode*      prev {nullptr};

        WriteSetNode() {
        	for (int i = 0; i < MAXLOGSIZE; i++) buckets[i] = &log[MAXLOGSIZE-1];
        }
    };

    struct WriteSetCL {
    	uint8_t*           addrCL {nullptr};
    	WriteSetCL*        next {nullptr};
    };

    // A single entry in the write-set
    struct WriteSetNodeCL {
        WriteSetCL*        buckets[HASH_BUCKETS];  // HashMap for CLWBs
        WriteSetCL         log[HASH_BUCKETS];
        WriteSetNodeCL*    next {nullptr};
        WriteSetNodeCL*    prev {nullptr};

        WriteSetNodeCL() {
        	for (int i = 0; i < HASH_BUCKETS; i++) buckets[i] = &log[HASH_BUCKETS-1];
        }
    };


    class State {
    public:
        std::atomic<SeqTidIdx> ticket {0};
        std::atomic<bool>      applied[MAX_THREADS];
        std::atomic<uint64_t>  results[MAX_THREADS];
        WriteSetNode           logHead {};
        WriteSetNode*          logTail {nullptr};
        uint64_t               lSize = 0;
        std::atomic<uint64_t>  logSize {0};

        WriteSetNodeCL         logHeadCL {};
        WriteSetNodeCL*        logTailCL {nullptr};
        uint64_t               numCL = 0;

        State() { // Used only by the first objState instance
            logTail = &logHead;
            logTailCL = &logHeadCL;
            for (int i = 0; i < MAX_THREADS; i++) applied[i].store(false, std::memory_order_relaxed);
            for (int i = 0; i < MAX_THREADS; i++) results[i].store(0, std::memory_order_relaxed);
        }
        ~State() {
            WriteSetNode* node = logHead.next;
            WriteSetNode* delNode = node;
            while(node!=nullptr){
                node = node->next;
                delete delNode;
                delNode = node;
            }

            WriteSetNodeCL* nodeCL = logHeadCL.next;
			WriteSetNodeCL* delNodeCL = nodeCL;
			while(nodeCL!=nullptr){
				nodeCL = nodeCL->next;
				delete delNodeCL;
				delNodeCL = nodeCL;
			}
        }

        // We can't use the "copy assignment operator" because we need to make sure that the instance
        // we're copying is the one we have protected with the hazard pointer
        void copyFrom(const State* from) {
        	std::atomic_thread_fence(std::memory_order_seq_cst);
            //const uint64_t numThreads = ThreadRegistry::getMaxThreads();
            for (int i = 0; i < MAX_THREADS; i++) applied[i].store(from->applied[i].load(std::memory_order_relaxed), std::memory_order_relaxed);
            for (int i = 0; i < MAX_THREADS; i++) results[i].store(from->results[i].load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
    };

    struct NodeCLAggr {
    	static const uint64_t  HASH_BUCKETS = 1024;
		WriteSetCL*            buckets[HASH_BUCKETS];  // HashMap for CLWBs aggregated
		WriteSetCL             log[HASH_BUCKETS];
		NodeCLAggr*            next {nullptr};
		NodeCLAggr*            prev {nullptr};

		NodeCLAggr() {
			for (int i = 0; i < HASH_BUCKETS; i++) buckets[i] = &log[HASH_BUCKETS-1];
		}
	};


    struct CLAggregate {
    	NodeCLAggr     logHeadCL {};
    	NodeCLAggr*    logTailCL {nullptr};
        uint64_t       numCL = 0;
        static const uint64_t numBuckets = NodeCLAggr::HASH_BUCKETS;
        CLAggregate() {
        	logTailCL = &logHeadCL;
		}

        inline uint64_t hash(const void* addr) const {
            return (((uint64_t)addr) >> 6) % NodeCLAggr::HASH_BUCKETS;
        }

        // add a new cache line to the hashmap is not present
		inline void addIfAbsent(void* cl) {
			const uint64_t hashAddr = hash(cl);
			NodeCLAggr* tail = logTailCL;
			if(numCL!=0){
				//3/4 of usedSize flush copy
		    	if(numCL+1 > 3*gRedo.esloco.getUsedSize()/(64*4)) {
		    		tlocal.copy = true;
		    		numCL = 0;
		    		return;
		    	}
				//const uint64_t numBuckets = NodeCLAggr::HASH_BUCKETS;
				int numCLnodes = (numCL%numBuckets==0)? numCL/numBuckets:numCL/numBuckets+1;

				NodeCLAggr* node = tail;

				for(int i=0;i<numCLnodes;i++){
					if(i == 32) break;
					WriteSetCL* be = node->buckets[hashAddr];

					if(hash(be->addrCL) != hashAddr){
						node = node->prev;
						continue;
					}

					if(i<numCLnodes-1 ||
					  (i == numCLnodes-1 && be < &node->log[numCL%numBuckets])){
						while (be != nullptr) {
							if (be->addrCL == cl) return;
							be = be->next;
						}
					}
					node = node->prev;
				}

				if(numCL%numBuckets==0){
					NodeCLAggr* next = tail->next;
					if(next ==nullptr){
						next = new NodeCLAggr();
						tail->next = next;
						next->prev = tail;
					}
					tail = next;
					logTailCL = next;
				}
			}
			WriteSetCL* e = &tail->log[numCL%numBuckets];

			e->addrCL = (uint8_t*)cl;
			// Add to hashmap
			WriteSetCL* be = tail->buckets[hashAddr];
			// Clear if entry is from previous tx
			e->next = (be < e && hash(be->addrCL) == hashAddr) ? be : nullptr;
			tail->buckets[hashAddr] = e;
			numCL++;
		}

	    inline void flushDeferredPWBs() {
	    	if(numCL==0) return;
	    	int numCLnodes = (numCL%numBuckets==0)?numCL/numBuckets:numCL/numBuckets+1;
			NodeCLAggr* nodeCL = &logHeadCL;
			int size = numBuckets;
			for(int i=0;i<numCLnodes;i++){
				if(i==numCLnodes-1){
					size = numCL%numBuckets;
					if(size==0) size = numBuckets;
				}
				for (int k = 0; k < size; k++) {
					PWB(nodeCL->log[k].addrCL + tlocal.tl_cx_size);
				}
				nodeCL = nodeCL->next;
			}
	    }

	    inline void reset() {
	    	numCL = 0;
	    }

	    inline void merge(State* state) {
	    	if(state->numCL==0) return;
	    	int numCLnodes = (state->numCL%HASH_BUCKETS==0)?state->numCL/HASH_BUCKETS:state->numCL/HASH_BUCKETS+1;
			WriteSetNodeCL* nodeCL = &state->logHeadCL;
			int size = HASH_BUCKETS;
			for(int i=0;i<numCLnodes;i++){
				if(i==numCLnodes-1){
					size = state->numCL%HASH_BUCKETS;
					if(size==0) size = HASH_BUCKETS;
				}
				for (int k = 0; k < size; k++) {
					addIfAbsent(nodeCL->log[k].addrCL);
				}
				nodeCL = nodeCL->next;
			}
	    }

	};

    // Class to combine head and the instance.
    // We must separately initialize each of the heaps in the heapPool because
    // they may be aligned differentely. The actual offset that the synthetic
    // pointers must use is not 'heapOffset' but 'ptrOffset', which is calculated
    // based on each Combined's 'ms'.
    struct Combined {
        std::atomic<SeqTidIdx>     head {0};
        uint8_t*                   root {nullptr}; //offset in bytes
        StrongTryRIRWLock          rwLock {MAX_THREADS};
        bool                       flushcopy{false};
        CLAggregate                clsets{};
    };

public:

    alignas(128) std::chrono::time_point<std::chrono::steady_clock> gstartTime {0us};



    static const int MAX_LINES = 256;
    uint64_t lines[MAX_THREADS][MAX_LINES];
    //uint64_t iline[MAX_THREADS] {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


    // Returns a uint64_t that is the combination of the Sequence, the Tid and Index
    inline SeqTidIdx makeSeqTidIdx(uint64_t seq, uint64_t tid, uint64_t idx) {
        return (seq << (TID_BITS+IDX_BITS)) | (tid << IDX_BITS) | idx;
    }
    // Returns the Sequence in a SeqTidIdx
    inline uint64_t sti2seq(SeqTidIdx sti) {
        return sti >> (TID_BITS+IDX_BITS);
    }
    // Returns the Tid in a SeqTidIdx
    inline uint64_t sti2tid(SeqTidIdx sti) {
        return (sti >> (IDX_BITS)) & ((1 << TID_BITS)-1);
    }
    // Returns the Idx in a SeqTidIdx
    inline uint64_t sti2idx(SeqTidIdx sti) {
        return sti & ((1 << IDX_BITS)-1);
    }



private:
    class States {
    public:
        State*       states;
        uint64_t      lastIdx{1};
        States() {
            states = new State[STATESSIZE];
        }
        ~States() {
            delete[] states;
        }
    };


    inline void apply_undolog(State* state) noexcept {
    	WriteSetNode* node = state->logTail;
        int len = state->lSize;
        int j=0;
        if(len>0){
            uint64_t offset = tlocal.tl_cx_size;
            len = len%MAXLOGSIZE;
            if(len == 0) len = MAXLOGSIZE;
            while(node!=nullptr){
                for(int i=len-1;i>=0;i--){
                    WriteSetEntry* entry = &node->log[i];
                    *(uint64_t*)( entry->addr + offset ) = entry->oldval;
                    j++;
                }
                node = node->prev;
                len=MAXLOGSIZE;
            }
        }
    }

    inline void copy_redolog(State* state, uint64_t redoSize, int tid, const uint64_t offset) noexcept {
        WriteSetNode* node= &state->logHead;
        int j=0;
        int len = MAXLOGSIZE;
        while(true){
            if(redoSize-j<MAXLOGSIZE) {
                len = redoSize%MAXLOGSIZE;
            }
            for(int i=0;i<len;i++){
            	WriteSetEntry e = node->log[i];
            	if(!ADDR_IS_IN_MAIN(e.addr)) return;
            	*(uint64_t*)(e.addr + offset) = e.val;
                j++;
            }
            if(j>=redoSize) return;
            node = node->next;
        }
    }

    inline Combined* apply_redologs(Combined* newComb, uint64_t initCombSeq, SeqTidIdx lastAppliedTicket, SeqTidIdx ltail, int tid) noexcept {
        uint64_t start = sti2seq(lastAppliedTicket);
        uint64_t i = start+1;
        uint64_t lastSeq = sti2seq(ltail);
        SeqTidIdx ringTicket;
        const uint64_t offset = tlocal.tl_cx_size;
        for(;i<=lastSeq;i++){
            ringTicket = ring[i%RINGSIZE].load();
            if(i != sti2seq(ringTicket)) break;
            State* apply_state = &sauron[sti2tid(ringTicket)].states[sti2idx(ringTicket)];

            const uint64_t redoSize = apply_state->logSize.load();
            if (ringTicket != apply_state->ticket.load()) {
                break;
            }
            //if redoSize bigger than 1/10 of used size abort and do copy
            if(redoSize > (esloco.getUsedSize()/(8*10))){
            	break;
            }
            if(redoSize > 0){
                copy_redolog(apply_state, redoSize, tid, offset);
                atomic_thread_fence(std::memory_order_acquire);
                if (ringTicket != apply_state->ticket.load()) {
                    break;
                }
            }
            if(sti2seq(per->curComb.load())>=initCombSeq+2){
            	//tmpclsets[tid].flushDeferredPWBs();
            	//PFENCE();
            	newComb->head.store(ringTicket,std::memory_order_relaxed);
            	return nullptr;
            }
        }

        // Failed to apply redo log so make a copy
        if(i != lastSeq+1){
        	if(!makeCopy(newComb, tid)) return nullptr;
        }else{
            newComb->head.store(ringTicket,std::memory_order_relaxed);
        }
        return newComb;
    }

    alignas(128) States* sauron;

    alignas(128) std::atomic<SeqTidIdx>* ring;

    alignas(128) Combined* combs;

    // Enqueue requests
    alignas(128) std::atomic<std::function<uint64_t()>*> enqueuers[MAX_THREADS];
    alignas(128) std::atomic<bool>                       announce[MAX_THREADS];

    // We need two hazard pointers for the enqueue() (ltail and lnext), one for myNode, and two to traverse the list/queue
    HazardPointers<std::function<uint64_t()>> hpMut {1, maxThreads};
    const int kHpMut     = 0;

    // Latest measurements of copy time
    alignas(128) std::atomic<microseconds> copyTime {100000us};

    inline int getCombined(const int tid) {
    	SeqTidIdx initComb = per->curComb.load();
    	const int initCombSeq = sti2seq(initComb);
        // 3 instead of 2 to avoid PFENCE when reading curComb
        for (int i = 0; i < 2; i++) {
            SeqTidIdx cComb = per->curComb.load();
            if(sti2seq(cComb) >= initCombSeq+2) break;
            const int curCombIndex = sti2idx(cComb);
            Combined* lcomb = &combs[curCombIndex];
            if (!lcomb->rwLock.sharedTryLock(tid)) continue;
            if (cComb == per->curComb.load()) return curCombIndex;
            lcomb->rwLock.sharedUnlock(tid);
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
        std::atomic<SeqTidIdx>   curComb {0};
        persist<void*>*    objects {nullptr};   // directory
#ifdef USE_ESLOCO
        void*              mspadd;
#else
        mspace             ms {};
#endif
        uint8_t            padding[1024-32]; // padding so that PersistentHeader size is 1024 bytes
    };

    PersistentHeader* per {nullptr};
public:
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

    /*
    bool copyFromTo(uint8_t* from , uint8_t* to, int fromIdx) {
    	auto startTime = steady_clock::now();
    	uint8_t* _from = from;
    	uint8_t* _to = to;
		uint64_t lcxsize = tlocal.tl_cx_size;
		tlocal.tl_cx_size = fromIdx * g_main_size;
		uint64_t usedSize = esloco.getUsedSize();
		tlocal.tl_cx_size = lcxsize;

		SeqTidIdx curC = per->curComb.load();
		uint64_t initCombSeq = sti2seq(curC);
		uint64_t size = usedSize;
		uint64_t copySize = 16*1024;

    	assert(((uint64_t)_from%8)==0);
    	assert(((uint64_t)size%8)==0);
    	const int ntsize = 8; // 1 word
		const int quadntsize = 8*8; // 8 words

		while(true){
			if((uint64_t)_from%quadntsize==0) break;
			ntstore(_to, _from);
			_from = _from + ntsize;
			_to = _to + ntsize;
			size = size - ntsize;
		}
		//copy
		while(size>0){
			if(copySize > size) {
				ntmemcpy(_to, _from, size);
				break;
			}
			if(std::memcmp(_to, _from, copySize)!=0){
				quadntmemcpy(_to, _from, copySize);
			}
			SeqTidIdx cComb = per->curComb.load();
			if(cComb != curC) {
				if(sti2seq(cComb) >= initCombSeq+2) return false;
	            Combined* lcomb = &combs[sti2idx(cComb)];
	            SeqTidIdx ltail = lcomb->head.load();
	            if(cComb!=per->curComb.load()) return false;

				States* tail_states = &sauron[sti2tid(ltail)];
				State* tail_state = &tail_states->states[sti2idx(ltail)];
				const int tid = ThreadRegistry::getTID();
				bool an = announce[tid].load(std::memory_order_relaxed);
				if(an == tail_state->applied[tid].load()){
					if(cComb == per->curComb.load()) return false;
				}

	            curC = cComb;
			}
			size = size - copySize;
			_to = _to+copySize;
			_from = _from+copySize;
		}
		auto endTime = steady_clock::now();
		microseconds timeus = duration_cast<microseconds>(endTime-startTime);
		copyTime.store(timeus, std::memory_order_release);
		return true;
    }*/


    bool copyFromTo(uint8_t* from , uint8_t* to, int fromIdx, uint64_t initComb, int tid) {
    	auto startTime = steady_clock::now();
    	uint8_t* _from = from;
    	uint8_t* _to = to;
		uint64_t lcxsize = tlocal.tl_cx_size;
		tlocal.tl_cx_size = fromIdx * g_main_size;
		uint64_t usedSize = esloco.getUsedSize();
		tlocal.tl_cx_size = lcxsize;

		uint64_t size = usedSize;
		uint64_t copySize = 16*1024;

    	assert(((uint64_t)_from%8)==0);
    	assert(((uint64_t)size%8)==0);
    	const int ntsize = 8; // 1 word
		const int quadntsize = 8*8; // 8 words


		while(size>0){
			if((uint64_t)_from%quadntsize==0) break;
			ntstore(_to, _from);
			_from = _from + ntsize;
			_to = _to + ntsize;
			size = size - ntsize;
		}
		//copy
		while(size>0){
			if(copySize > size) {
				ntmemcpy(_to, _from, size);
				break;
			}
			if(std::memcmp(_to, _from, copySize)!=0){
				quadntmemcpy(_to, _from, copySize);
			}
			if(per->curComb.load() != initComb) {
				return false;
			}

			size = size - copySize;
			_to = _to+copySize;
			_from = _from+copySize;
		}
		//prevents reordering between per->curComb.load() and std::memcmp(_to, _from, copySize) and quadntmemcpy(_to, _from, copySize)
		std::atomic_thread_fence(std::memory_order_seq_cst);
		if(per->curComb.load() != initComb) {
			return false;
		}
		auto endTime = steady_clock::now();
		microseconds timeus = duration_cast<microseconds>(endTime-startTime);
		copyTime.store(timeus, std::memory_order_release);
		return true;
    }

    void ntmemcpy(uint8_t* _to, uint8_t* _from, uint64_t copySize){
		const int ntsize = 8;
		uint8_t* ptr = _from;
		uint8_t* last = _from + copySize;
		uint8_t* _dst = _to;
		for (; ptr < last; ptr += ntsize) {
			ntstore(_dst, ptr);
			_dst = _dst + ntsize;
		}
    }

    void quadntmemcpy(uint8_t* _to, uint8_t* _from, uint64_t copySize){
    	const int ntsize = 8; // 1 word
		const int quadntsize = 8*8; // 8 words
		uint8_t* ptr = _from;
		uint8_t* last = _from + copySize;
		uint8_t* _dst = _to;

		for (; ptr < last; ptr += quadntsize) {
			quadntstore(_dst, ptr);
			_dst = _dst + quadntsize;
		}
    }

    bool flushCopy(uint8_t* to, uint64_t usedSize){
    	// flush pwb
    	SeqTidIdx curC = per->curComb.load();
    	uint64_t initCombSeq = sti2seq(curC);
    	uint64_t size = usedSize;
    	//check every 64 pwbs
    	uint64_t flushSize = 4096;
    	while(size>0){
			if(flushSize > size) flushSize = size;
			flush_range(to, flushSize);
			SeqTidIdx cComb = per->curComb.load();
			if(cComb != curC) {
				if(sti2seq(cComb) >= initCombSeq+2) return false;
				Combined* lcomb = &combs[sti2idx(cComb)];
				SeqTidIdx ltail = lcomb->head.load();
				if(cComb!=per->curComb.load()) return false;

				States* tail_states = &sauron[sti2tid(ltail)];
				State* tail_state = &tail_states->states[sti2idx(ltail)];
				const int tid = ThreadRegistry::getTID();
				bool an = announce[tid].load(std::memory_order_relaxed);
				if(an == tail_state->applied[tid].load()){
					if(cComb == per->curComb.load()) return false;
				}

				curC = cComb;
			}
			size = size - flushSize;
			to = to+flushSize;
    	}
    	return true;
    }


public:
    /*
     * addr -> address in Main region (destination = addr+offset)
     * cpyaddr -> address of copy from data (source)
     */
    inline void dbLog(void* addr, void* cpyaddr, size_t size) noexcept {
        void* _addr = addr;
        void* _cpyaddr = cpyaddr;
        uint64_t offset = tlocal.tl_cx_size;
        if(size == 0) return;
        while(true){
            uint64_t oldval = *reinterpret_cast<uint64_t*>( (uint8_t*)_addr + offset );
            uint64_t newval = *reinterpret_cast<uint64_t*>(_cpyaddr);
            gRedo.addAddrIfAbsent(_addr, oldval, newval);
            if(size > 8) {
                _addr = (uint8_t*)_addr+8;
                _cpyaddr = (uint8_t*)_cpyaddr+8;
                size = size-8;
            }else break;
        }
    }

    inline void dbFlush(uint8_t* addr, size_t size) noexcept {
        uint8_t* _addr = addr;
        uint64_t offset = tlocal.tl_cx_size;
        if(size == 0) return;
        while(true){
            uint64_t* cl = (uint64_t*)ADDR2CL(_addr);
            //DEFER_PWB(cl);
            gRedo.addIfAbsent(cl);
            if(size>=64){
                _addr =_addr+64;
                size = size-64;
            }
            else {
                uint64_t* endcl = (uint64_t*)ADDR2CL((uint8_t*)(_addr+size-1));
                if(cl!=endcl){
                    //DEFER_PWB(endcl);
                    gRedo.addIfAbsent(endcl);
                }
                break;
            }
        }
    }

    inline void flushDeferredPWBs(State* state) {
    	if(state->numCL==0) return;
    	int numCLnodes = (state->numCL%HASH_BUCKETS==0)?state->numCL/HASH_BUCKETS:state->numCL/HASH_BUCKETS+1;
		WriteSetNodeCL* nodeCL = &state->logHeadCL;

		int size = HASH_BUCKETS;
		for(int i=0;i<numCLnodes;i++){
			if(i==numCLnodes-1){
				if(state->numCL%HASH_BUCKETS!=0){
					size = state->numCL%HASH_BUCKETS;
				}
			}
			for (int k = 0; k < size; k++) {
				PWB(nodeCL->log[k].addrCL + tlocal.tl_cx_size);
			}
			nodeCL = nodeCL->next;
		}
    }

    // Each cl on a different bucket
	inline uint64_t hashAddress(const void* addr) const {
		return ((uint64_t)addr>>3) % MAXLOGSIZE;
	}

	inline void addAddress(WriteSetNode* tail, uint64_t lSize, uint8_t* addr,
			uint64_t oldval, uint64_t val, const uint64_t hashAddr, WriteSetEntry* next){

		WriteSetEntry* e = &tail->log[lSize%MAXLOGSIZE];
		e->addr = (uint8_t*)addr;
		e->oldval = oldval;
		e->val = val;
		// Clear if entry is from previous tx
		e->next = next;
		tail->buckets[hashAddr] = e;
	}

    /*
     * @return false if address is already present otherwise true
     */
    inline bool addAddrIfAbsent(void* addr, uint64_t oldval, uint64_t val) noexcept {
    	State* state = (State*)tlocal.st;
    	const uint64_t hashAddr = hashAddress(addr);
    	WriteSetNode* tail = state->logTail;
    	uint64_t lSize = state->lSize;
    	if(lSize==0){
    		addAddress(tail, lSize, (uint8_t*)addr, oldval, val, hashAddr,nullptr);
    		state->lSize=lSize+1;
			return true;
    	}
    	WriteSetNode* node = tail;
    	if(lSize%MAXLOGSIZE==0){
    		for(int i=0;i<lSize/MAXLOGSIZE;i++){
    			if(i==16) break;
    			WriteSetEntry* be = node->buckets[hashAddr];
				if(hashAddress(be->addr) == hashAddr){
					while (be != nullptr) {
						if (be->addr == addr) {
							be->val = val;
							return false;
						}
						be = be->next;
					}
				}
    			node = node->prev;
    		}

			WriteSetNode* next = tail->next;
			if(next ==nullptr){
				next = new WriteSetNode();
				tail->next = next;
				next->prev = tail;
			}
			tail = next;
			state->logTail = next;

    	}else{
    		int numNodes = lSize/MAXLOGSIZE +1;
    		for(int i=0;i<numNodes;i++){
    			if(i==16) break;
				WriteSetEntry* be = node->buckets[hashAddr];
				if(hashAddress(be->addr) != hashAddr){
					node = node->prev;
					continue;
				}
				if(i != 0 ||
				  (i == 0 &&
					be < &node->log[lSize%MAXLOGSIZE])){

					while (be != nullptr) {
						if (be->addr == addr) {
							be->val = val;
							return false;
						}
						be = be->next;
					}
				}
				node = node->prev;
			}
    	}
    	WriteSetEntry* e = &tail->log[lSize%MAXLOGSIZE];
    	WriteSetEntry* be = tail->buckets[hashAddr];
		// Clear if entry is from previous tx
    	WriteSetEntry* next = (be < e && hashAddress(be->addr) == hashAddr) ? be : nullptr;
    	addAddress(tail, lSize, (uint8_t*)addr, oldval, val, hashAddr,next);
		state->lSize=lSize+1;
		return true;
    }

    // Each cl on a different bucket
    inline uint64_t hash(const void* addr) const {
        return (((uint64_t)addr) >> 6) % HASH_BUCKETS;
    }

    // add a new cache line to the hashmap is not present
    inline void addIfAbsent(void* addr) {
    	State* state = (State*)tlocal.st;
    	if(tlocal.copy == true) return;
    	uint8_t* cl = ADDR2CL(addr);
        const uint64_t hashAddr = hash(cl);
    	WriteSetNodeCL* tail = state->logTailCL;
    	uint64_t numCL = state->numCL;
    	if(numCL==0){
    		WriteSetCL* e = &tail->log[numCL];
            e->addrCL = cl;
            e->next = nullptr;
            tail->buckets[hashAddr] = e;
            state->numCL++;
			return;
    	}

    	if(numCL+1 > 3*esloco.getUsedSize()/(64*4)) {
    		tlocal.copy = true;
    		state->numCL = 0;
    		return;
    	}

        WriteSetNodeCL* node = tail;

		if(numCL%HASH_BUCKETS==0){
			for(int i=0;i<numCL/HASH_BUCKETS;i++){
				if(i==16) break;
				WriteSetCL* be = node->buckets[hashAddr];
				if(hash(be->addrCL) != hashAddr){
					be = nullptr;
				}
				while (be != nullptr) {
					if (be->addrCL == cl) return;
					be = be->next;
				}
				node = node->prev;
			}
			WriteSetNodeCL* next = tail->next;
			if(next ==nullptr){
				next = new WriteSetNodeCL();
				tail->next = next;
				next->prev = tail;
			}
			tail = next;
			state->logTailCL = next;


		}else{
			int numCLnodes = numCL/HASH_BUCKETS +1;
			for(int i=0;i<numCLnodes;i++){
				if(i==16) break;
				WriteSetCL* be = node->buckets[hashAddr];
				if(hash(be->addrCL) != hashAddr){
					node = node->prev;
					continue;
				}
				if(i!=0 ||
				  (i == 0 && be < &node->log[numCL%HASH_BUCKETS])){
					while (be != nullptr) {
						if (be->addrCL == cl) return;
						be = be->next;
					}
				}
				node = node->prev;
			}
		}


		WriteSetCL* e = &tail->log[numCL%HASH_BUCKETS];

        e->addrCL = cl;
        //printf("addrCL %p %p \n",e->addrCL,addr);
        // Add to hashmap
        WriteSetCL* be = tail->buckets[hashAddr];
        // Clear if entry is from previous tx
        e->next = (be < e && hash(be->addrCL) == hashAddr) ? be : nullptr;
        tail->buckets[hashAddr] = e;
        state->numCL++;
    }


    RedoOpt() : dommap{true},maxThreads{MAX_THREADS}{
    	gstartTime = steady_clock::now();
        sauron = new States[maxThreads];
        ring = new std::atomic<SeqTidIdx>[RINGSIZE];
        for(int i=0;i<RINGSIZE;i++) ring[i] = 0;
        combs = new Combined[MAX_COMBINEDS];
        for (int i = 0; i < maxThreads; i++) enqueuers[i].store(nullptr, std::memory_order_relaxed);
        tlocal.writes = new uint64_t[REGISTRY_MAX_THREADS];
        for (int i = 0; i < REGISTRY_MAX_THREADS; i++) tlocal.writes[i]=0;
        NUM_CORES = std::thread::hardware_concurrency();
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
                g_main_size = (g_main_size/1024)*1024; // Round of g_main_size to a multiple of 1024
                g_main_addr = base_addr + sizeof(PersistentHeader);
                g_main_addr_end = g_main_addr + g_main_size;
                g_region_end = g_main_addr +MAX_COMBINEDS*g_main_size;
                uint64_t combidx = sti2idx(per->curComb.load());
                for(int i = 0; i < MAX_COMBINEDS; i++){
                	combs[i].root = g_main_addr + i*g_main_size;
                	if(i!=combidx){
						combs[i].head.store(makeSeqTidIdx(0, 1, 0),std::memory_order_relaxed);
                	}else{
                		combs[i].head.store(0,std::memory_order_relaxed);
                	}
                }
                Combined* comb = &combs[combidx];
                comb->rwLock.setReadLock();
                per->curComb.store(makeSeqTidIdx(0, 0, combidx));
                //std::cout<<per->curComb.load()<<" curComb\n";
                #ifdef USE_ESLOCO
                    esloco.init(g_main_addr, g_main_size, false);
                #endif
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
        g_main_size = (g_main_size/1024)*1024; // Round of g_main_size to a multiple of 1024
        g_main_addr = base_addr + sizeof(PersistentHeader);
        g_main_addr_end = g_main_addr + g_main_size;
        g_region_end = g_main_addr +MAX_COMBINEDS*g_main_size;
        PWB(&per->curComb);

        for(int i = 0; i < MAX_COMBINEDS; i++){
            combs[i].root = g_main_addr + i*g_main_size;
        }
        Combined* comb = &combs[sti2idx(per->curComb.load())];
        comb->rwLock.setReadLock();
        updateTx<bool>([&] () {
#ifdef USE_ESLOCO
            esloco.init(g_main_addr, g_main_size, true);
            per->objects = (persist<void*>*)esloco.malloc(sizeof(void*)*NUM_OBJS);
#else
            per->ms = create_mspace_with_base(g_main_addr, g_main_size, false);
            per->objects = (persist<void*>*)mspace_malloc(per->ms, sizeof(void*)*NUM_OBJS);
#endif
            for (int i = 0; i < NUM_OBJS; i++) {
                per->objects[i] = nullptr;
            }
            return true;
        });
        PFENCE();
        per->id = MAGIC_ID;
        PWB(&per->id);
        PSYNC();
    }


    ~RedoOpt() {
        printf("Currently used PM = %ld MB\n", esloco.getUsedSize()/(1024*1024));

        uint64_t totalReplicas = 0;
        for (int i = 0; i < MAX_COMBINEDS; i++) if (combs[i].head != 0) totalReplicas++;
        printf("Number of used replicas = %ld\n", totalReplicas);

        delete[] sauron;
        delete[] ring;
        delete[] combs;

        // Must do munmap() if we did mmap()
        if (dommap) {
            //destroy_mspace(ms);
            munmap(base_addr, max_size);
            close(fd);
        }
        if (enableAllocStatistics) { // Show leak report
            std::cout << "RedoOpt: statsAllocBytes = " << statsAllocBytes << "\n";
            std::cout << "RedoOpt: statsAllocNum = " << statsAllocNum << "\n";
        }
    }

    static std::string className() { return "RedoOptPTM"; }

    template <typename T>
    static inline T* get_object(int idx) {
        return reinterpret_cast<T*>( gRedo.per->objects[idx].pload() );
    }

    template <typename T>
    static inline void put_object(int idx, T* obj) {
        gRedo.per->objects[idx].pstore(obj);
    }


    template<typename R,class F>
    R ns_read_transaction(F&& func) {
        if (tl_nested_read_trans > 0) {
            return (R)func();
        }
        int tid = ThreadRegistry::getTID();
        ++tl_nested_read_trans;
        for (int i=0; i < MAX_READ_TRIES + 2; i++) {

            if (i == MAX_READ_TRIES) { // enqueue read-only operation as if it was a mutation
                auto oldfunc = enqueuers[tid].load(std::memory_order_relaxed);
                std::function<uint64_t()>* myfunc = (std::function<uint64_t()>*)new std::function<R()>(func);
                enqueuers[tid].store(myfunc, std::memory_order_relaxed);
                if (oldfunc != nullptr) hpMut.retire(oldfunc, tid);
                const bool newrequest = !announce[tid].load();
                announce[tid].store(newrequest); // seq-cst store
            }
            SeqTidIdx cComb = per->curComb.load();
            const int curCombIndex = sti2idx(cComb);
            Combined* lcomb = &combs[curCombIndex];

            if (lcomb->rwLock.sharedTryLock(tid)) {

            	if(cComb == per->curComb.load()){
            		SeqTidIdx ticket = lcomb->head.load();
					if (sti2seq(ticket) == sti2seq(cComb)) {
						tlocal.tl_cx_size = curCombIndex*g_main_size;
						auto ret = func();
						lcomb->rwLock.sharedUnlock(tid);
						SeqTidIdx ringtail = ring[sti2seq(ticket)%RINGSIZE].load();

						if(sti2seq(ringtail) < sti2seq(ticket)){
							PWB(&per->curComb);
							PSYNC();
						}
						--tl_nested_read_trans;
						tlocal.tl_cx_size = 0;
						return (R)ret;
					}
            	}
                lcomb->rwLock.sharedUnlock(tid);
            }
        }
        --tl_nested_read_trans;
        SeqTidIdx cComb = per->curComb.load();
        PWB(&per->curComb);
        PSYNC();

		const int combIndex = sti2idx(cComb);
		Combined* comb = &combs[combIndex];
		SeqTidIdx t = comb->head.load();
		SeqTidIdx combSeq = sti2seq(cComb);
		if(sti2seq(t)!=combSeq){
			t = ring[combSeq%RINGSIZE].load();
		}

        State* tstate = &sauron[sti2tid(t)].states[sti2idx(t)];

        return (R)tstate->results[tid].load();
    }

    bool makeCopy(Combined* newComb, int tid){
    	newComb->clsets.reset();
    	SeqTidIdx initComb = per->curComb.load();
    	uint64_t initCombSeq = sti2seq(initComb);
    	newComb->head.store(makeSeqTidIdx(0, 1, 0));
    	for(int i=0;i<2;i++){
    		int lCombIndex = sti2idx(initComb);
    		Combined* lcomb = &combs[lCombIndex];

            SeqTidIdx head = lcomb->head.load();
            if(initComb!=per->curComb.load()) {
				initComb = per->curComb.load();
				if(sti2seq(initComb)>= initCombSeq+2) return false;
				continue;
            }

        	States* tail_states = &sauron[sti2tid(head)];
    		State* tail_state = &tail_states->states[sti2idx(head)];
    		bool an = announce[tid].load(std::memory_order_relaxed);

    		if(an == tail_state->applied[tid].load()){
    			if(initComb==per->curComb.load()) return false;
				initComb = per->curComb.load();
				if(sti2seq(initComb)>= initCombSeq+2) return false;
				continue;
    		}

			if(!copyFromTo(lcomb->root, newComb->root, lCombIndex, initComb, tid)){
				initComb = per->curComb.load();
				if(sti2seq(initComb)>= initCombSeq+2) return false;
				continue;
			}
			newComb->flushcopy = false;
			tlocal.copy = false;
			//newComb->head.store(lcomb->head.load());
			newComb->head.store(head);
			return true;
    	}
    	assert(false);
    	return false;
    }
/*
    bool makeCopy(Combined* newComb, int tid){
    	int lCombIndex;
		if((lCombIndex = getCombined(tid)) == -1) {
			newComb->head.store(makeSeqTidIdx(0, 1, 0),std::memory_order_relaxed);
			return false;
		}
		newComb->clsets.reset();
		Combined* lcomb = &combs[lCombIndex];
		if(!copyFromTo(lcomb->root, newComb->root, lCombIndex)){
			newComb->head.store(makeSeqTidIdx(0, 1, 0),std::memory_order_relaxed);
			lcomb->rwLock.sharedUnlock(tid);
			return false;
		}
		newComb->head.store(lcomb->head.load(),std::memory_order_relaxed);
		lcomb->rwLock.sharedUnlock(tid);
		newComb->flushcopy = false;
		tlocal.copy = false;
		return true;
    }*/
/*
    int getNewComb(uint64_t cComb, const int tid) {
    	unsigned int mThreads = ThreadRegistry::getMaxThreads();
    	if(mThreads>1){
    		SeqTidIdx curC = per->curComb.load();
    		int start = sti2idx(curC)+1;
    		for(int i=start;i<MAX_COMBS;i++){
            	SeqTidIdx curC = per->curComb.load();
                if (cComb!=curC) return -1;
                if (combs[i].rwLock.exclusiveTryLock(tid)) return i;
    		}
    	}

    	auto startTime = steady_clock::now();

    	if(copyTime.load()==0us){
            for (int i = 0; i < MAX_COMBINEDS; i++) {
            	SeqTidIdx curC = per->curComb.load();
                if (cComb!=curC) return -1;
                if (combs[i].rwLock.exclusiveTryLock(tid)) return i;
            }
    	}

        // Now yield until time is over (or half the time?)
        auto endTime = steady_clock::now();
        microseconds timeus = duration_cast<microseconds>(endTime-startTime);

        while (timeus < copyTime.load()*4) {
            for (int i = 0; i < MAX_COMBS; i++) {
            	SeqTidIdx curC = per->curComb.load();
            	if (cComb!=curC) return -1;
                if (combs[i].rwLock.exclusiveTryLock(tid)) return i;
            }
            if( mThreads > NUM_CORES )
            	std::this_thread::yield();
            endTime = steady_clock::now();
            timeus = duration_cast<microseconds>(endTime-startTime);
        }
        // Now scan to the end (there can be multiple ones repeating on the first MAX_COMBS instances)
        for (int i = 0; i < MAX_COMBINEDS; i++) {
        	SeqTidIdx curC = per->curComb.load();
        	if (cComb!=curC) return -1;
            if (combs[i].rwLock.exclusiveTryLock(tid)) return i;
        }
        return -1;
    }*/


    int getNewComb(uint64_t cComb, const int tid) {
    	/*
    	unsigned int mThreads = ThreadRegistry::getMaxThreads();
    	if(mThreads>1){
    		SeqTidIdx curC = per->curComb.load();
    		int start = sti2idx(curC)+1;
    		for(int i=start;i<MAX_COMBS;i++){
            	SeqTidIdx curC = per->curComb.load();
                if (cComb!=curC) return -1;
                if (combs[i].rwLock.exclusiveTryLock(tid)) return i;
    		}
    	}*/

        auto startTime = steady_clock::now();
        int maxCombs = MAX_COMBS;
        auto endTime = steady_clock::now();
        microseconds timeus = duration_cast<microseconds>(endTime-startTime);
        while(true){
			while (timeus < copyTime.load()*100 || copyTime.load()==0us) {
				for (int i = 0; i < maxCombs; i++) {
	            	SeqTidIdx curC = per->curComb.load();
	                if (cComb!=curC) return -1;
					if (combs[i].rwLock.exclusiveTryLock(tid)) return i;
				}
				std::this_thread::yield();
				endTime = steady_clock::now();
				timeus = duration_cast<microseconds>(endTime-startTime);
			}
			maxCombs++;
			startTime = endTime;
			timeus = 0us;
			assert(maxCombs<=maxThreads+1);
        }
        return -1;
    }

    // Non-static thread-safe read-write transaction
    template<typename R,class F>
    R ns_write_transaction(F&& func) {
        const int tid = ThreadRegistry::getTID();
        if (tl_nested_write_trans > 0) return (R)func();
        ++tl_nested_write_trans;
        auto oldfunc = enqueuers[tid].load(std::memory_order_relaxed);
        std::function<uint64_t()>* myfunc = (std::function<uint64_t()>*)new std::function<R()>(func);
        enqueuers[tid].store(myfunc, std::memory_order_relaxed);
        if (oldfunc != nullptr) hpMut.retire(oldfunc, tid);

        const bool newrequest = !announce[tid].load(std::memory_order_relaxed);
        announce[tid].store(newrequest);
        uint64_t initCombSeq = sti2seq(per->curComb.load());

        Combined* newComb = nullptr;
        int newCombIndex = 0;
        States* newStates = &sauron[tid];
        State* newState = &newStates->states[newStates->lastIdx];
        //used for logging
        tlocal.st = newState;
        for (int iter = 0; iter < 2; iter++) {
            SeqTidIdx cComb = per->curComb.load();
            uint64_t seqltail = sti2seq(cComb);
            Combined* lcomb = &combs[sti2idx(cComb)];
            SeqTidIdx ltail = lcomb->head.load();
            if(seqltail >= initCombSeq+2) break;

            if (cComb != per->curComb.load()) continue;
            States* tail_states = &sauron[sti2tid(ltail)];
            State* tail_state = &tail_states->states[sti2idx(ltail)];

            if(newrequest == tail_state->applied[tid].load()){
				if(cComb == per->curComb.load()) break;
				continue;
			}

            SeqTidIdx newTicket = makeSeqTidIdx(seqltail+1, (uint64_t)tid, newStates->lastIdx);
            newState->ticket.store(newTicket);
            newState->logTail = &newState->logHead;
            newState->lSize = 0;
            newState->numCL = 0;
            newState->logTailCL = &newState->logHeadCL;
            // Copy the contents of the current State into the new State
            newState->copyFrom(tail_state);
            newState->logSize.store(0);

            if(cComb != per->curComb.load()) continue;
            SeqTidIdx ringtail = ring[seqltail%RINGSIZE].load();
            if(ltail != ringtail){
                if(sti2seq(ringtail) > seqltail) continue;
                PWB(&per->curComb);
                //advance tail like Michael and Scott
                ring[seqltail%RINGSIZE].compare_exchange_strong(ringtail, ltail);
            }

            if(newComb == nullptr){
				newCombIndex = getNewComb(cComb, tid);
				if(newCombIndex==-1) continue;
            }
            newComb = &combs[newCombIndex];
            tlocal.tl_cx_size = newCombIndex*g_main_size;

            //apply missing redo log
            SeqTidIdx lastAppliedTicket = newComb->head.load();

            if(lastAppliedTicket == makeSeqTidIdx(0, 1, 0)){
				if(!makeCopy(newComb, tid)) break;
			}else{
				tlocal.copy = newComb->flushcopy;
				if( apply_redologs(newComb, initCombSeq, lastAppliedTicket, ltail, tid)==nullptr) break;
			}

            // re-start because curComb changed
            if(cComb != per->curComb.load()) continue;

            //optimization
            //if (seqltail != sti2seq(ring[seqltail%RINGSIZE].load())) break;

            // Now newComb is as up to date as curComb
            // Help other requests, starting from zero

            bool atleastone = false;
            int numberofwrites = 0;
			for (uint64_t i = 0; i < maxThreads; i++) {
				// Check if it is an open request
				bool applied = newState->applied[i].load();
				if (announce[i].load() == applied) continue;
				// Apply the mutation and save the result
				std::function<uint64_t()>* mutation = hpMut.protectPtr(kHpMut, enqueuers[i].load(), tid);
				if (mutation != enqueuers[i].load()) break;
				if(cComb != per->curComb.load()) break;

				atleastone = true;
				newState->results[i].store((*mutation)(),std::memory_order_release);
				newState->applied[i].store(!applied);
				numberofwrites++;
			}

			if(!atleastone) continue;
			if(!tlocal.copy){
				newComb->clsets.merge(newState);
			}else{
				newComb->flushcopy = tlocal.copy;
			}
            if(cComb != per->curComb.load()){
            	apply_undolog(newState);
            	continue;
            }

			if(tlocal.copy){
				if(!flushCopy(newComb->root, esloco.getUsedSize())){
					apply_undolog(newState);
					break;
				}
				newComb->flushcopy = false;
				tlocal.copy = false;
			}else{
				newComb->clsets.flushDeferredPWBs();
			}
			newComb->clsets.reset();

            newState->logSize.store(newState->lSize,std::memory_order_relaxed);
            newComb->head.store(newTicket,std::memory_order_relaxed);

            newComb->rwLock.downgrade();
            SeqTidIdx newcComb = makeSeqTidIdx(seqltail+1, (uint64_t)tid, (uint64_t)newCombIndex);
            //this PFENCE is ommited because it will be executed per->curComb.compare_exchange_strong(tmp, newCombIndex) that
            // will issue the necessary fence that orders previous PWB and the store on curComb
            //PFENCE();
#ifdef MEASURE_PWB
            tl_num_pfences++;
#endif
            if (per->curComb.compare_exchange_strong(cComb, newcComb)){
                lcomb->rwLock.setReadUnlock();
                SeqTidIdx oldTicket = ring[(seqltail+1)%RINGSIZE].load();
                if(sti2seq(oldTicket) < seqltail+1){
                	PWB(&per->curComb);
                	//PSYNC();
#ifdef MEASURE_PWB
                	tl_num_pfences++;
#endif
                    ring[(seqltail+1)%RINGSIZE].compare_exchange_strong(oldTicket, newTicket);
                }
                newStates->lastIdx++;
                if(newStates->lastIdx == STATESSIZE) newStates->lastIdx = 0;
                hpMut.clear(tid);
                --tl_nested_write_trans;
#ifdef MEASURE_PWB
                tlocal.writes[numberofwrites]++;
#endif
                tlocal.tl_cx_size = 0;
                tlocal.st = nullptr;
                return (R)newState->results[tid].load();
            }
            apply_undolog(newState);
            newComb->head.store(ltail,std::memory_order_release);
            newComb->rwLock.setReadUnlock();
            newComb = nullptr;
        }
        hpMut.clear(tid);
        if(newComb!=nullptr){
            newComb->rwLock.exclusiveUnlock();
        }
        --tl_nested_write_trans;
        tlocal.tl_cx_size = 0;
        tlocal.st = nullptr;

		SeqTidIdx cComb = per->curComb.load();
		const int combIndex = sti2idx(cComb);
		Combined* comb = &combs[combIndex];
		SeqTidIdx t = comb->head.load();
		SeqTidIdx combSeq = sti2seq(cComb);
		if(sti2seq(t)!=combSeq){
			t = ring[combSeq%RINGSIZE].load();
		}else{
			SeqTidIdx oldTicket = ring[combSeq%RINGSIZE].load();
			if(sti2seq(oldTicket) < combSeq){
				PWB(&per->curComb);
				//PSYNC();
				ring[combSeq%RINGSIZE].compare_exchange_strong(oldTicket, t);
			}
		}
        States* tstates = &sauron[sti2tid(t)];
        State* tstate = &tstates->states[sti2idx(t)];

        return (R)tstate->results[tid].load();
    }


    /*
     * Mean to be called from user code when something bad happens and the whole
     * transaction needs to be aborted.
     * This function has strict semantics.
     */
    inline void abort_transaction(void);


    template <typename T, typename... Args> static T* tmNew(Args&&... args) {
        RedoOpt& r = gRedo;
#ifdef USE_ESLOCO
        void* addr = r.esloco.malloc(sizeof(T));
        assert(addr != nullptr);
#else
        void* addr = mspace_malloc( ((uint8_t*)(r.per->ms))+tlocal.tl_cx_size, sizeof(T));
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
        RedoOpt& r = gRedo;
#ifdef USE_ESLOCO
        r.esloco.free(obj);
#else
        mspace_free( ((uint8_t*)(r.per->ms))+tlocal.tl_cx_size,obj);
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
        RedoOpt& r = gRedo;
#ifdef USE_ESLOCO
        void* addr = r.esloco.malloc(size);
        assert(addr != nullptr);
#else
        void* addr = mspace_malloc( ((uint8_t*)(r.per->ms))+tlocal.tl_cx_size , size);
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
        RedoOpt& r = gRedo;
#ifdef USE_ESLOCO
        r.esloco.free(ptr);
#else
        mspace_free( ((uint8_t*)(r.per->ms))+tlocal.tl_cx_size, ptr);
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
        return gRedo.ns_read_transaction<R>(func);
    }

    template<typename R,class F> inline static R updateTx(F&& func) {
        return gRedo.ns_write_transaction<R>(func);
    }
    //template<typename F> static void readTx(F&& func) { gCX.ns_read_transaction<R>(func); }
    //template<typename F> static void updateTx(F&& func) { gCX.ns_write_transaction(func); }
    // Doesn't actually do any checking. That functionality exists only for RomulusLog and RomulusLR
    static bool consistency_check(void) {
        return true;
    }
};

template<typename T> inline void persist<T>::pstore(T newVal) {
        uint8_t* valaddr = (uint8_t*)&val;
        uint64_t offset = tlocal.tl_cx_size;
        bool sameAddr = false;
        if (offset != 0 && ADDR_IS_IN_MAIN(valaddr)) {
        	uint64_t oldval = (uint64_t)*reinterpret_cast<T*>( valaddr + offset );
            if(oldval != (uint64_t)newVal) {
            	sameAddr = !gRedo.addAddrIfAbsent(valaddr, oldval, (uint64_t)newVal);
            	*reinterpret_cast<T*>( valaddr + offset ) = newVal;
            }
           if(!tlocal.copy && !sameAddr) gRedo.addIfAbsent(valaddr);
        } else if (ADDR_IS_IN_REGION(valaddr)) {
                if((uint64_t)val != (uint64_t)newVal){
                	sameAddr = !gRedo.addAddrIfAbsent(valaddr - offset, (uint64_t)val, (uint64_t)newVal);
                	val = newVal;
                }
                if(!tlocal.copy && !sameAddr) gRedo.addIfAbsent(valaddr - offset);
        }else{
        	val = newVal;
        }
}
}
#endif   // _REDOOPT_PERSISTENCY_H_
