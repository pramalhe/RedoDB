// Wait-free array bounded queue with RedoOpt PTM engine
//
// Build this with:
// g++-8 -O3 -g -DPWB_IS_CLFLUSH -I.. example2.cpp ../ptms/redotimedhash/RedoTimedHash.cpp ../common/ThreadRegistry.cpp -o example2

#include <stdio.h>
#include "../pdatastructures/pqueues/TMArrayQueue.hpp"
#include "../ptms/redoopt/RedoOpt.hpp"

int main(void) {
    typedef TMArrayQueue<uint64_t,40,redotimedhash::RedoTimedHash,redotimedhash::persist> PQueue;

    // Create an empty queue in PM and save the root pointer (index 0) to use in a later tx
    redotimedhash::updateTx<bool>([=] () {
        printf("Creating persistent array-backed queue...\n");
        PQueue* pqueue = redotimedhash::tmNew<PQueue>();
        redotimedhash::put_object(0, pqueue);
        return true;
    });

    // Add two items to the persistent queue
    redotimedhash::updateTx<bool>([=] () {
        PQueue* pqueue = redotimedhash::get_object<PQueue>(0);
        pqueue->enqueue(33);
        pqueue->enqueue(44);
        return true;
    });

    // dequeue two items from the persistent queue
    redotimedhash::updateTx<bool>([=] () {
        PQueue* pqueue = redotimedhash::get_object<PQueue>(0);
        printf("Poped two items: %ld and %ld\n", pqueue->dequeue(), pqueue->dequeue());
        // This one should be "EMTPY" which is 0
        printf("Poped one more: %ld\n", pqueue->dequeue());
        return true;
    });

    // Delete the persistent queue from persistent memory
    redotimedhash::updateTx<bool>([=] () {
        printf("Destroying persistent queue...\n");
        PQueue* pqueue = redotimedhash::get_object<PQueue>(0);
        redotimedhash::tmDelete(pqueue);
        redotimedhash::put_object<PQueue>(0, nullptr);
        return true;
    });


    return 0;
}

