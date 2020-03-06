// Wait-free array bounded queue with OneFileWF PTM engine
//
// Build this with:
// g++-8 -O3 -g -DPWB_IS_CLFLUSH -I.. example1.cpp -o example1

#include <stdio.h>
#include "../ptms/ponefilewf/OneFilePTMWF.hpp"
#include "../pdatastructures/pqueues/TMArrayQueue.hpp"

int main(void) {
    typedef TMArrayQueue<uint64_t,40,onefileptmwf::OneFileWF,onefileptmwf::tmtype> PQueue;

    // Create an empty queue in PM and save the root pointer (index 0) to use in a later tx
    onefileptmwf::updateTx<bool>([=] () {
        printf("Creating persistent array-backed queue...\n");
        PQueue* pqueue = onefileptmwf::tmNew<PQueue>();
        onefileptmwf::put_object(0, pqueue);
        return true;
    });

    // Add two items to the persistent queue
    onefileptmwf::updateTx<bool>([=] () {
        PQueue* pqueue = onefileptmwf::get_object<PQueue>(0);
        pqueue->enqueue(33);
        pqueue->enqueue(44);
        return true;
    });

    // dequeue two items from the persistent queue
    onefileptmwf::updateTx<bool>([=] () {
        PQueue* pqueue = onefileptmwf::get_object<PQueue>(0);
        printf("Poped two items: %ld and %ld\n", pqueue->dequeue(), pqueue->dequeue());
        // This one should be "EMTPY" which is 0
        printf("Poped one more: %ld\n", pqueue->dequeue());
        return true;
    });

    // Delete the persistent queue from persistent memory
    onefileptmwf::updateTx<bool>([=] () {
        printf("Destroying persistent queue...\n");
        PQueue* pqueue = onefileptmwf::get_object<PQueue>(0);
        onefileptmwf::tmDelete(pqueue);
        onefileptmwf::put_object<PQueue>(0, nullptr);
        return true;
    });


    return 0;
}

