/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#include "RomulusLR.hpp"


namespace romuluslr{
    /*
     * <h1> RomulusLR </h1>
     * TODO: explain this...
     *
     */

// Global with the 'main' size. Used by pload()
uint64_t g_main_size = 0;
// Global with the 'main' addr. Used by pload()
uint8_t* g_main_addr = 0;

uint8_t* g_main_addr_end;
// Instance with current ongoing transaction
RomulusLR* romlr = nullptr;

alignas(128) bool g_right = false;

thread_local int tl_lrromulus = 0;
// Counter of nested write transactions
thread_local int64_t tl_nested_write_trans = 0;
// Counter of nested read-only transactions
thread_local int64_t tl_nested_read_trans = 0;

RomulusLR gRomLR {};

extern mspace create_mspace_with_base(void* base, size_t capacity, int locked);


void RomulusLR::copyMainToBack() {
    // Copy the data from 'main' to 'back'
    uint64_t size = std::min(per->used_size, g_main_size);
    std::memcpy(back_addr, main_addr, size);
    flush_range(back_addr, size);
}

void RomulusLR::copyBackToMain() {
    // Copy the data from 'back' to 'main'
    uint64_t size = std::min(per->used_size, g_main_size);
    std::memcpy(main_addr, back_addr, size);
    flush_range(main_addr, size);
}


bool RomulusLR::compareMainAndBack() {
    if (std::memcmp(main_addr, back_addr, g_main_size) != 0) {
        void* firstaddr = nullptr;
        int sumdiff = 0;
        for (size_t idx = 0; idx < g_main_size-sizeof(size_t); idx++) {
            if (*(main_addr+idx) != *(back_addr+idx)) {
                printf("Difference at %p  main=%ld  back=%ld\n", main_addr+idx, *(int64_t*)(main_addr+idx), *(int64_t*)(back_addr+idx));
                sumdiff++;
                if (firstaddr == nullptr) firstaddr = main_addr+idx;
            }
        }
        if (sumdiff != 0) {
            printf("sumdiff=%d bytes\n", sumdiff);
            printf("\nThere seems to be a missing persist<T> in your code.\n");
            printf("Rerun with gdb and set a watchpoint using the command\nwatch * %p\n\n", firstaddr);
        }
        assert(sumdiff == 0);
    }
    return true;
}

RomulusLR::RomulusLR() : dommap{true},maxThreads{128}{

    fc = new std::atomic< std::function<void()>* >[maxThreads*CLPAD];
    for (int i = 0; i < maxThreads; i++) {
        fc[i*CLPAD].store(nullptr, std::memory_order_relaxed);
    }
    romlr = this;
    if (dommap) {
        base_addr = (uint8_t*)0x7fdd80000000;
        max_size = PM_REGION_SIZE;
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
            if (per->id != MAGIC_ID) createFile();
            g_main_size = (max_size - sizeof(PersistentHeader))/2;
            main_addr = base_addr + sizeof(PersistentHeader);
            back_addr = main_addr + g_main_size;
            g_main_addr = main_addr;
            g_main_addr_end = main_addr + g_main_size;
            g_right = false;
            recover();
        } else {
            createFile();
        }
    }
}


RomulusLR::~RomulusLR() {
    delete[] fc;
    // Must do munmap() if we did mmap()
    if (dommap) {
        //destroy_mspace(ms);
        munmap(base_addr, max_size);
        close(fd);
    }
}

void RomulusLR::createFile(){
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
    g_main_size = (max_size - sizeof(PersistentHeader))/2;
    main_addr = base_addr + sizeof(PersistentHeader);
    back_addr = main_addr + g_main_size;
    g_main_addr = main_addr;
    g_main_addr_end = main_addr + g_main_size;

    // We need to call create_mspace_with_base() from within a transaction so that
    // the modifications on 'main' get replicated on 'back'. This means we temporarily
    // need to set the 'used_size' to 'main_size' to make sure everything is copied.
    g_right = false;
    begin_transaction();

    // Just to force the copy of the whole main region
    per->used_size = g_main_size;
    per->ms = create_mspace_with_base(main_addr, g_main_size, false);
    per->objects = (void**)mspace_malloc(per->ms, sizeof(void*)*100);
    for (int i = 0; i < 100; i++) {
        per->objects[i] = nullptr;
        add_to_log(&per->objects[i],sizeof(void*));
        PWB(&per->objects[i]);
    }
    end_transaction();

    // The used bytes in the main region
    per->used_size = (uint8_t*)(&per->used_size) - ((uint8_t*)base_addr+sizeof(PersistentHeader))+128;
    flush_range((uint8_t*)per,sizeof(PersistentHeader));
    PFENCE();
    // Finally, set the id to confirm that the whole initialization process has completed
    per->id = MAGIC_ID;
    PWB(&per->id);
    PSYNC();

}



/*
 * Recovers from an incomplete transaction if needed
 */
inline void RomulusLR::recover() {
    int lstate = per->state.load(std::memory_order_relaxed);
    if (lstate == IDLE) {
        return;
    } else if (lstate == COPYING) {
        printf("RomulusLR: Recovery from COPYING...\n");
        copyMainToBack();
    } else if (lstate == MUTATING) {
        printf("RomulusLR: Recovery from MUTATING...\n");
        copyBackToMain();
    } else {
        assert(false);
        // ERROR: corrupted state
    }
    PFENCE();
    per->state.store(IDLE, std::memory_order_relaxed);
    return;
}
}
