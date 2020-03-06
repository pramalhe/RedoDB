/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#include <cstring>

/*
 * EsLoco is an Extremely Simple memory aLOCatOr
 *
 * It is based on intrusive singly-linked lists (a free-list), one for each power of two size.
 * All blocks are powers of two, the smallest size enough to contain the desired user data plus the block header.
 * There is an array named 'freelists' where each entry is a pointer to the head of a stack for that respective block size.
 * Blocks are allocated in powers of 2 of words (64bit words).
 * Each block has an header with two words: the size of the node (in words), the pointer to the next node.
 * The minimum block size is 4 words, with 2 for the header and 2 for the user.
 * When there is no suitable block in the freelist, it will create a new block from the remaining pool.
 *
 * EsLoco was designed for usage in PTMs but it doesn't have to be used only for that.
 * Average number of stores for an allocation is 1.
 * Average number of stores for a de-allocation is 2.
 *
 * Memory layout:
 * ------------------------------------------------------------------------
 * | poolTop | freelists[0] ... freelists[61] | ... allocated objects ... |
 * ------------------------------------------------------------------------
 */
template <template <typename> class P>
class EsLoco {
private:
    struct block {
        P<block*>   next;   // Pointer to next block in free-list (when block is in free-list)
        P<uint64_t> size;   // Exponent of power of two of the size of this block in bytes.
    };

    const bool debugOn = false;

    // Volatile data
    uint8_t* poolAddr {nullptr};
    uint64_t poolSize {0};

    // Volatile pointer to array of persistent heads of free-list (blocks)
    block* freelists {nullptr};
    // Volatile pointer to persistent pointer to last unused address (the top of the pool)
    P<uint8_t*>* poolTop {nullptr};

    // Number of blocks in the freelists array.
    // Each entry corresponds to an exponent of the block size: 2^4, 2^5, 2^6... 2^40
    static const int kMaxBlockSize = 40; // 1 TB of memory should be enough

    // For powers of 2, returns the highest bit, otherwise, returns the next highest bit
    uint64_t highestBit(uint64_t val) {
        uint64_t b = 0;
        while ((val >> (b+1)) != 0) b++;
        if (val > (1 << b)) return b+1;
        return b;
    }

    uint8_t* aligned(uint8_t* addr) {
        return (uint8_t*)((size_t)addr & (~0x3FULL)) + 128;
    }

public:
    void init(void* addressOfMemoryPool, size_t sizeOfMemoryPool, bool clearPool=true) {
        // Align the base address of the memory pool

        poolAddr = (uint8_t*)addressOfMemoryPool;
        if(poolAddr!=(uint8_t*)addressOfMemoryPool)std::cout<<"ERROR init Esloco \n";
        poolSize = sizeOfMemoryPool + (uint8_t*)addressOfMemoryPool - poolAddr;
        // The first thing in the pool is a pointer to the top of the pool
        poolTop = (P<uint8_t*>*)poolAddr;
        // The second thing in the pool is the array of freelists
        freelists = (block*)(poolAddr + sizeof(*poolTop));
        if (clearPool) {
            // Unlike OneFile, in CX and Redo we don't need to clear the pool with memset()
            for (int i = 0; i < kMaxBlockSize; i++) freelists[i].next.pstore(nullptr);
            // The size of the freelists array in bytes is sizeof(block)*kMaxBlockSize
            // Align to cache line boundary
            poolTop->pstore(aligned(poolAddr + sizeof(*poolTop) + sizeof(block)*kMaxBlockSize));
        }
        if (debugOn) printf("Starting EsLoco with poolAddr=%p and poolSize=%ld, up to %p\n", poolAddr, poolSize, poolAddr+poolSize);
    }

    // Resets the metadata of the allocator back to its defaults
    void reset() {
        std::memset(poolAddr, 0, sizeof(block)*kMaxBlockSize);
        poolTop->pstore(nullptr);
    }

    // Returns the number of bytes that may (or may not) have allocated objects, from the base address to the top address
    uint64_t getUsedSize() {
    	if(poolAddr==nullptr) return 0;//to deal with transaction in createFile()
        return poolTop->pload() - poolAddr;
    }

    // Takes the desired size of the object in bytes.
    // Returns pointer to memory in pool, or nullptr.
    // Does on average 1 store to persistent memory when re-utilizing blocks.
    void* malloc(size_t size) {
        P<uint8_t*>* top = (P<uint8_t*>*)(((uint8_t*)poolTop));
        // Adjust size to nearest (highest) power of 2
        uint64_t bsize = highestBit(size + sizeof(block));
        if (debugOn) printf("malloc(%ld) requested,  block size exponent = %ld\n", size, bsize);
        block* myblock = nullptr;
        // Check if there is a block of that size in the corresponding freelist
        if (freelists[bsize].next.pload() != nullptr) {
            if (debugOn) printf("Found available block in freelist\n");
            // Unlink block
            myblock = freelists[bsize].next.pload();
            freelists[bsize].next = myblock->next.pload();// pstore()
        } else {
            if (debugOn) printf("Creating new block from top, currently at %p\n", top->pload());
            // Couldn't find a suitable block, get one from the top of the pool if there is one available
            if (top->pload() + (1<<bsize) > poolSize + poolAddr) return nullptr;
            myblock = (block*)top->pload();
            top->pstore(top->pload() + (1<<bsize));      // pstore()
            myblock->size = bsize;                       // pstore()
        }
        if (debugOn) printf("returning ptr = %p\n", (void*)((uint8_t*)myblock + sizeof(block)));
        // Return the block, minus the header
        return (void*)((uint8_t*)myblock + sizeof(block));
    }

    // Takes a pointer to an object and puts the block on the free-list.
    // Does on average 2 stores to persistent memory.
    void free(void* ptr) {
        if (ptr == nullptr) return;
        block* myblock = (block*)((uint8_t*)ptr - sizeof(block));
        if (debugOn) printf("free(%p)  block size exponent = %ld\n", ptr, myblock->size.pload());
        // Insert the block in the corresponding freelist
        uint64_t size = myblock->size.pload();
        myblock->next = freelists[size].next.pload();    // pstore()
        freelists[size].next = myblock;                  // pstore()
    }
};
