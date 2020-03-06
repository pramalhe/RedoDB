/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _REDO_PERSISTENCY_H_
#define _REDO_PERSISTENCY_H_

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

#include "../../common/pfences.h"
#include "../../common/ThreadRegistry.hpp"
#include "../../common/StrongTryRIRWLock.hpp"
#include "../redo/HazardPointers.hpp"

// TODO: take this out if we decide to go with Doug Lea's malloc for CX
#define USE_ESLOCO_REDO

// Default size (in bytes) of the full memory mapped region used by this PTM.
// This can also be changed in the Makefile
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
#define PM_FILE_NAME   "/dev/shm/redo_shared"
#endif


namespace redo {


/*
 * <h1> CX Persistent Transactional Memory </h1>
 * TODO: explain this...
 *
 *
 *
 */

// Forward declaration of CX to create a global instance
class Redo;
extern Redo gRedo;

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

//extern thread_local void* st;
//extern thread_local uint64_t tl_cx_size;

// Debug this in gdb with:
// p lines[tid][(iline[tid]-0)%MAX_LINES]
// p lines[tid][(iline[tid]-1)%MAX_LINES]
// p lines[tid][(iline[tid]-2)%MAX_LINES]
#define DEBUG_LINE() lines[tid][iline[tid]%MAX_LINES] = __LINE__; iline[tid]++;

#define ADDR_IS_IN_MAIN(addr) ((uint8_t*)(addr) >= g_main_addr && (uint8_t*)(addr) < g_main_addr_end)
#define ADDR_IS_IN_REGION(addr) ((addr) >= g_main_addr && (addr) < g_region_end)

#ifdef USE_ESLOCO_REDO
#include "../../common/EsLoco.hpp"
#else // Use Doug Lea's malloc
// This macro is used in Doug Lea's malloc to adjust weird castings that don't call the '&' operator
#define CX_ADJUSTED_ADDR(addr) (ADDR_IS_IN_REGION(addr)) ? (malloc_chunk*)(addr - tlocal.tl_cx_size) : (malloc_chunk*)addr

typedef void* mspace;
extern void* mspace_malloc(mspace msp, size_t bytes);
extern void mspace_free(mspace msp, void* mem);
extern mspace create_mspace_with_base(void* base, size_t capacity, int locked);
#endif

// Log of the deferred PWBs
#define PWB_LOG_SIZE  (2048*32)
extern thread_local void* tl_pwb_log[PWB_LOG_SIZE];
//extern thread_local int tl_pwb_idx;
#define DEFER_PWB(_addr) if (tlocal.tl_pwb_idx < PWB_LOG_SIZE) { tl_pwb_log[tlocal.tl_pwb_idx++] = (_addr); } else { gRedo.flushDeferredPWBs(); PWB(_addr);}
// Returns the cache line of the address (this is for x86 only)
#define ADDR2CL(_addr) (uint8_t*)((size_t)(_addr) & (~63ULL))

struct varLocal {
	void* st{};
	uint64_t tl_cx_size{0};
	int tl_pwb_idx{0};
	int64_t tl_nested_write_trans{0};
	int64_t tl_nested_read_trans{0};
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

class Redo {
    static const int MAX_READ_TRIES = 10; // Maximum number of times a reader will fail to acquire the shared lock before adding its operation as a mutation
    static const int MAX_THREADS = 65;
    static const int MAX_COMBINEDS = 128;
    static const int NUM_OBJS = 100;
    static const int MAXLOGSIZE = 256;
    static const int RINGSIZE = 16192;
    static const int STATESSIZE = 4096;
    // Constants for SeqTidIdx. They must sum to 64 bits
    static const int SEQ_BITS = 44;
    static const int TID_BITS = 8;   // Can't have more than 256 threads (should be enough for now)
    static const int IDX_BITS = 12;  // WARNING: ring capacity up to 4096
    const int maxThreads;

    // Class to combine head and the instance.
    // We must separately initialize each of the heaps in the heapPool because
    // they may be aligned differentely. The actual offset that the synthetic
    // pointers must use is not 'heapOffset' but 'ptrOffset', which is calculated
    // based on each Combined's 'ms'.
    struct Combined {
        std::atomic<SeqTidIdx>     head {0};
        uint8_t*                   root {nullptr}; //offset in bytes
        StrongTryRIRWLock          rwLock {MAX_THREADS};
    };

    // A single entry in the write-set
    struct WriteSetEntry {
        uint8_t*       addr {nullptr};      // Address of value+sequence to change
        uint64_t       oldval {0};          // Previous value
        uint64_t       val {0};             // Desired value to change to
    };

    // A single entry in the write-set
    struct WriteSetNode {
        WriteSetEntry      log[MAXLOGSIZE];     // Redo log of stores
        WriteSetNode*      next {nullptr};
        WriteSetNode*      prev {nullptr};
    };

    static const int MAX_LINES = 256;
    uint64_t lines[MAX_THREADS][MAX_LINES];
    uint64_t iline[MAX_THREADS] {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    WriteSetEntry** tmpwsets {nullptr};

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




public:

    class State {
    public:
        std::atomic<SeqTidIdx> ticket {0};
        std::atomic<bool>      applied[MAX_THREADS];
        std::atomic<uint64_t>  results[MAX_THREADS];
        WriteSetNode           logHead {};
        WriteSetNode*          logTail {nullptr};
        uint64_t               lSize = 0;
        std::atomic<uint64_t>  logSize {0};
        State() { // Used only by the first objState instance
            logTail = &logHead;
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


    inline void apply_undologPWB(State* state) noexcept {
        int len = state->lSize;
        if(len>0){
            uint64_t offset = tlocal.tl_cx_size;
            len = len%MAXLOGSIZE;
            if(len == 0) len = MAXLOGSIZE;
            WriteSetNode* node = state->logTail;
            while(node!=nullptr){
                for(int i=len-1;i>=0;i--){
                    WriteSetEntry* entry = &node->log[i];
                    *(uint64_t*)( entry->addr + offset ) = entry->oldval;
                    DEFER_PWB(ADDR2CL( entry->addr + offset ));
                }
                node = node->prev;
                len=MAXLOGSIZE;
            }
        }
    }

    inline void copy_redolog(State* state, uint64_t redoSize, WriteSetEntry* entries, int tid) noexcept {
        WriteSetNode* node= &state->logHead;
        int j=0;
        int len = MAXLOGSIZE;
        while(true){
            if(redoSize-j<MAXLOGSIZE) {
                len = redoSize%MAXLOGSIZE;
            }
            for(int i=0;i<len;i++){
                entries[j] = node->log[i];
                j++;
            }
            if(j>=redoSize) {
            	//assert(j==redoSize);
            	return;
            }
            node = node->next;
        }
    }

    inline Combined* apply_redologs(Combined* newComb, uint64_t initCombSeq, SeqTidIdx lastAppliedTicket, SeqTidIdx ltail, int tid) noexcept {
        uint64_t start = sti2seq(lastAppliedTicket);
        uint64_t i = start+1;
        uint64_t lastSeq = sti2seq(ltail);
        SeqTidIdx ringTicket;
        for(;i<=lastSeq;i++){
            ringTicket = ring[i%RINGSIZE].load();
            if(i != sti2seq(ringTicket)) break;
            State* apply_state = &sauron[sti2tid(ringTicket)].states[sti2idx(ringTicket)];

            const uint64_t redoSize = apply_state->logSize.load();

            if(redoSize > 0){
                WriteSetEntry* entries = tmpwsets[tid];
                // If the redo log is too large to fit in the temporary writeset, allocate a new one
                if (redoSize > MAXLOGSIZE) entries = new WriteSetEntry[redoSize];
                copy_redolog(apply_state, redoSize, entries, tid);
                atomic_thread_fence(std::memory_order_acquire);
                if (ringTicket != apply_state->ticket.load()) {
                    if (entries != tmpwsets[tid]) delete[] entries;
                    break;
                }
                // Apply the modifications and (defer) flush them with pwbs
                const uint64_t offset = tlocal.tl_cx_size;
                for(int j=0;j<redoSize;j++){
                    *(uint64_t*)(entries[j].addr + offset) = entries[j].val;
                    DEFER_PWB(ADDR2CL(entries[j].addr + offset));
                }
                if (entries != tmpwsets[tid]) delete[] entries;
            }
            //lastAppliedTicket = ringTicket;
            if(sti2seq(per->curComb.load())>=initCombSeq+2){
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
#ifdef USE_ESLOCO_REDO
        void*              mspadd;
#else
        mspace             ms {};
#endif
        uint8_t            padding[1024-32]; // padding so that PersistentHeader size is 1024 bytes
    };

    PersistentHeader* per {nullptr};

#ifdef USE_ESLOCO_REDO
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

    bool copyFromTo(uint8_t* from , uint8_t* to, int fromIdx) {
    	uint8_t* _from = from;
    	uint8_t* _to = to;
    	tlocal.tl_pwb_idx=0;
		uint64_t lcxsize = tlocal.tl_cx_size;
		tlocal.tl_cx_size = fromIdx * g_main_size;
		uint64_t usedSize = esloco.getUsedSize();
		tlocal.tl_cx_size = lcxsize;

		SeqTidIdx curC = per->curComb.load();
		uint64_t initCombSeq = sti2seq(curC);
		uint64_t size = usedSize;
		uint64_t flushSize = 4*1024;
		//copy
		while(size>0){
			if(flushSize > size) flushSize = size;
			std::memcpy(_to, _from, flushSize);
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
			_to = _to+flushSize;
			_from = _from+flushSize;
		}

		// flush pwb
		flush_range(to, usedSize);
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
            //printf("size=%ld\n", size);
			uint64_t oldval = *reinterpret_cast<uint64_t*>( (uint8_t*)_addr + offset );
			uint64_t newval = *reinterpret_cast<uint64_t*>(_cpyaddr);
			add_to_log((Redo::State*)tlocal.st, _addr, oldval,newval);
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
			DEFER_PWB(cl);
			if(size>=64){
				_addr =_addr+64;
				size = size-64;
			}
			else {
				uint64_t* endcl = (uint64_t*)ADDR2CL((uint8_t*)(_addr+size-1));
				if(cl!=endcl){
					DEFER_PWB(endcl);
				}
				break;
			}
    	}
    }

    inline void flushDeferredPWBs() {
        for (int k = 0; k < tlocal.tl_pwb_idx; k++) {
            PWB(tl_pwb_log[k]);
        }
        tlocal.tl_pwb_idx=0;
    }

    inline void cleanPWBs() {
        tlocal.tl_pwb_idx=0;
    }

    inline int findAddrInLog(State* state, void* addr, WriteSetNode*& tail) noexcept {
        int len = state->lSize;
        size_t addrCL = (size_t)ADDR2CL((uint8_t*)addr);
        bool sameCL = false;
        if(len>0){
            len = len%MAXLOGSIZE;
            if(len == 0) len = MAXLOGSIZE;
            WriteSetNode* node = state->logTail;
            int j=0;
            while(node!=nullptr && j<32){
                for(int i=len-1;i>=0;i--){
                    WriteSetEntry* entry = &node->log[i];
                    size_t entryCL = (size_t)ADDR2CL((uint8_t*)entry->addr);
                    if(addrCL == entryCL){
                    	if(addr == entry->addr) {
                    		tail = node;
                    		return i;
                    	}
                    	sameCL = true;
                    }
                    j++;
                    if(j>=32) break;
                }
                node = node->prev;
                len=MAXLOGSIZE;
            }
        }
        if(sameCL) return -2;
        return -1;
    }

    /*
     * Adds to the log the current contents of the memory location starting at
     * 'addr' with a certain 'length' in bytes
     */
    inline bool add_to_log(State* state, void* addr, uint64_t oldval, uint64_t val) noexcept {
    	WriteSetNode* tail = nullptr;
    	int pos = findAddrInLog(state,addr, tail);
    	if(pos<0){
    		tail = state->logTail;
			int len = state->lSize;
			int size = len;

			len = len%MAXLOGSIZE;
			if(len==0 && size>=MAXLOGSIZE){
				WriteSetNode* next = tail->next;
				if(next ==nullptr){
					//std::cout<<"tail "<<size<<"\n";
					next = new WriteSetNode();
					tail->next = next;
					next->prev = tail;
				}
				tail = next;
				state->logTail = next;
			}
			//std::cout<<"add "<<addr<<"\n";
			WriteSetEntry* entry = &tail->log[len];
			entry->addr = (uint8_t*)addr;
			entry->oldval = oldval;
			entry->val = val;
			state->lSize=size+1;
			if(pos==-2) return true;
			return false;
    	}else{
    		WriteSetEntry* entry = &tail->log[pos];
    		//std::cout<<"not add "<<entry->addr<<"\n";
			entry->val = val;
			return true;
    	}
    }

    Redo() : dommap{true},maxThreads{MAX_THREADS}{
        sauron = new States[maxThreads];
        ring = new std::atomic<SeqTidIdx>[RINGSIZE];
        for(int i=0;i<RINGSIZE;i++) ring[i] = 0;
        combs = new Combined[MAX_COMBINEDS];
        for (int i = 0; i < maxThreads; i++) enqueuers[i].store(nullptr, std::memory_order_relaxed);
        tmpwsets = new WriteSetEntry*[MAX_THREADS];
        for (int i = 0; i < maxThreads; i++) tmpwsets[i] = new WriteSetEntry[MAXLOGSIZE];

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

                for(int i = 0; i < MAX_COMBINEDS; i++){
                    combs[i].root = g_main_addr + i*g_main_size;
                }
                Combined* comb = &combs[sti2idx(per->curComb.load())];
                comb->rwLock.setReadLock();

                #ifdef USE_ESLOCO_REDO
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
#ifdef USE_ESLOCO_REDO
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


    ~Redo() {
        delete[] sauron;
        delete[] ring;
        delete[] combs;
        for (int i = 0; i < maxThreads; i++) delete[] tmpwsets[i];
        delete[] tmpwsets;

        // Must do munmap() if we did mmap()
        if (dommap) {
            //destroy_mspace(ms);
            munmap(base_addr, max_size);
            close(fd);
        }
        if (enableAllocStatistics) { // Show leak report
            std::cout << "Redo: statsAllocBytes = " << statsAllocBytes << "\n";
            std::cout << "Redo: statsAllocNum = " << statsAllocNum << "\n";
        }
    }

    static std::string className() { return "RedoPTM"; }

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
                std::function<uint64_t()>* myfunc = (std::function<uint64_t()>*)new std::function<R()>(std::forward<F>(func));
                enqueuers[tid].store(myfunc, std::memory_order_relaxed);
                if (oldfunc != nullptr) hpMut.retire(oldfunc, tid);
                const bool newrequest = !announce[tid].load();
                announce[tid].store(newrequest); // seq-cst store
            }
            SeqTidIdx cComb = per->curComb.load();
            const int curCombIndex = sti2idx(cComb);
            Combined* lcomb = &combs[curCombIndex];

            if (lcomb->rwLock.sharedTryLock(tid)) {
            	SeqTidIdx ticket = lcomb->head.load();
                if (sti2seq(ticket) == sti2seq(cComb) && cComb == per->curComb.load()) {
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
                lcomb->rwLock.sharedUnlock(tid);
            }
        }
        --tl_nested_read_trans;
        PWB(&per->curComb);
        PSYNC();
		SeqTidIdx cComb = per->curComb.load();
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
    	int lCombIndex;
		if((lCombIndex = getCombined(tid)) == -1) {
			newComb->head.store(makeSeqTidIdx(0, 1, 0),std::memory_order_relaxed);
			return false;
		}
		Combined* lcomb = &combs[lCombIndex];
		if(!copyFromTo(lcomb->root, newComb->root, lCombIndex)){
			newComb->head.store(makeSeqTidIdx(0, 1, 0),std::memory_order_relaxed);
			lcomb->rwLock.sharedUnlock(tid);
			return false;
		}
		newComb->head.store(lcomb->head.load(),std::memory_order_relaxed);
		lcomb->rwLock.sharedUnlock(tid);
		return true;
    }

    // Non-static thread-safe read-write transaction
    template<typename R,class F>
    R ns_write_transaction(F&& func) {
        const int tid = ThreadRegistry::getTID();
        if (tl_nested_write_trans > 0) return (R)func();
        ++tl_nested_write_trans;
        auto oldfunc = enqueuers[tid].load(std::memory_order_relaxed);
        std::function<uint64_t()>* myfunc = (std::function<uint64_t()>*)new std::function<R()>(std::forward<F>(func));
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

            SeqTidIdx newTicket = makeSeqTidIdx(seqltail+1, (uint64_t)tid, newStates->lastIdx);
            newState->ticket.store(newTicket);
            newState->logTail = &newState->logHead;
            newState->lSize = 0;
            // Copy the contents of the current State into the new State
            newState->copyFrom(tail_state);
            newState->logSize.store(0);

            // Check if my mutation has been applied
            if(cComb != per->curComb.load()) continue;
            SeqTidIdx ringtail = ring[seqltail%RINGSIZE].load();
            if(ltail != ringtail){
                if(sti2seq(ringtail) > seqltail) continue;
                //advance tail like Michael and Scott
                ring[seqltail%RINGSIZE].compare_exchange_strong(ringtail, ltail);
            }


            if(newComb == nullptr){
                for (int i = 0; i < MAX_COMBINEDS; i++) {
                	SeqTidIdx curC = per->curComb.load();
                	if (sti2seq(curC)>=initCombSeq+2) break;
                    if (combs[i].rwLock.exclusiveTryLock(tid)) {
                        newComb = &combs[i];
                        newCombIndex = i;
                        tlocal.tl_cx_size = newCombIndex*g_main_size;
                        break;
                    }
                }
            }
            if(newComb==nullptr) break;
            // Reset the deferred pwb log
            tlocal.tl_pwb_idx = 0;
            //apply missing redo log
            SeqTidIdx lastAppliedTicket = newComb->head.load();
            if(lastAppliedTicket == makeSeqTidIdx(0, 1, 0)){
            	if(!makeCopy(newComb, tid)) break;
            }else{
            	if( apply_redologs(newComb, initCombSeq, lastAppliedTicket, ltail, tid)==nullptr) break;
            }
            // Check if my mutation has been applied
            if(newrequest == tail_state->applied[tid].load()){
				if(cComb == per->curComb.load()) break;
				//request was applied. Even if state was reused it means curComb advanced RINGSIZE times.
				continue;
			}
            // re-start because curComb changed
            if(cComb != per->curComb.load()) continue;
            flushDeferredPWBs();
            //optimization
            //if (seqltail != sti2seq(ring[seqltail%RINGSIZE].load())) break;

            // Now newComb is as up to date as curComb
            // Help other requests, starting from zero

            bool atleastone = false;
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
            }

            if(atleastone) {
            	if (cComb != per->curComb.load()) {
            		cleanPWBs();
					apply_undologPWB(newState);
					continue;
            	}
            }
            else continue;

            // Execute deferred PWB()s of the modifications
            flushDeferredPWBs();
            // curComb must be flushed before we make our changes visible (on curComb)
            PWB(&per->curComb);
            newState->logSize.store(newState->lSize,std::memory_order_relaxed);
            newComb->head.store(newTicket,std::memory_order_relaxed);

            newComb->rwLock.downgrade();
            SeqTidIdx newcComb = makeSeqTidIdx(sti2seq(cComb)+1, (uint64_t)tid, (uint64_t)newCombIndex);
            //this PFENCE is ommited because it will be executed per->curComb.compare_exchange_strong(tmp, newCombIndex) that
            // will issue the necessary fence that orders previous PWB and the store on curComb
            //PFENCE();
            if (per->curComb.compare_exchange_strong(cComb, newcComb)){
                lcomb->rwLock.setReadUnlock();
                SeqTidIdx oldTicket = ring[(seqltail+1)%RINGSIZE].load();
                if(sti2seq(oldTicket) < seqltail+1){
                	PWB(&per->curComb);
                	//PSYNC();
                    ring[(seqltail+1)%RINGSIZE].compare_exchange_strong(oldTicket, newTicket);
                }
                newStates->lastIdx++;
                if(newStates->lastIdx == STATESSIZE) newStates->lastIdx = 0;
                hpMut.clear(tid);
                --tl_nested_write_trans;
                tlocal.tl_cx_size = 0;
                tlocal.st = nullptr;
                return (R)newState->results[tid].load();
            }
            apply_undologPWB(newState);
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
        Redo& r = gRedo;
#ifdef USE_ESLOCO_REDO
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
        Redo& r = gRedo;
#ifdef USE_ESLOCO_REDO
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
        Redo& r = gRedo;
#ifdef USE_ESLOCO_REDO
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
        Redo& r = gRedo;
#ifdef USE_ESLOCO_REDO
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
        return gRedo.ns_read_transaction<R>(std::forward<F>(func));
    }

    template<typename R,class F> inline static R updateTx(F&& func) {
        return gRedo.ns_write_transaction<R>(std::forward<F>(func));
    }
    //template<typename F> static void readTx(F&& func) { gRedo.ns_read_transaction<R>(func); }
    //template<typename F> static void updateTx(F&& func) { gRedo.ns_write_transaction(func); }
    // Doesn't actually do any checking. That functionality exists only for RomulusLog and RomulusLR
    static bool consistency_check(void) {
        return true;
    }
};

// We don't need to check currTx here because we're not de-referencing
// the val. It's only after a load() that the val may be de-referenced
// (in user code), therefore we do the check on load() only.
template<typename T> inline void persist<T>::pstore(T newVal) {
        uint8_t* valaddr = (uint8_t*)&val;
        uint64_t offset = tlocal.tl_cx_size;
        bool sameCL = false;
        if (offset != 0 && ADDR_IS_IN_MAIN(valaddr)) {
        	uint64_t oldval = (uint64_t)*reinterpret_cast<T*>( valaddr + offset );
        	sameCL = gRedo.add_to_log((Redo::State*)tlocal.st, valaddr, oldval, (uint64_t)newVal);
            if(oldval != (uint64_t)newVal) {
            	*reinterpret_cast<T*>( valaddr + offset ) = newVal;
            }
           if(!sameCL) DEFER_PWB(ADDR2CL(valaddr + offset));
        } else {
            if (ADDR_IS_IN_REGION(valaddr)) {
            	sameCL = gRedo.add_to_log((Redo::State*)tlocal.st, valaddr - offset, (uint64_t)val, (uint64_t)newVal);
                if((uint64_t)val != (uint64_t)newVal){
                	val = newVal;
                }
                if(!sameCL) DEFER_PWB(ADDR2CL(valaddr));
            }else{
                val = newVal;
            }

        }
}
}
#endif   // _Redo_PERSISTENCY_H_
