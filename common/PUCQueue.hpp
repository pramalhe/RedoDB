/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _PERSISTENT_UNIVERSAL_CONSTRUCT_QUEUE_H_
#define _PERSISTENT_UNIVERSAL_CONSTRUCT_QUEUE_H_

#include <atomic>
#include <stdexcept>
#include <cstdint>
#include <functional>

/**
 * <h1> Interface for Persistent Universal Constructs (Queues) </h1>
 *
 * UC is the Universal Construct
 * Q is the queue class
 * QItem is the type of the item in the queue
 */

// This class can be used to simplify the usage of queues with Universal Constructs
// For generic code you don't need it (and can't use it), but it can serve as an example of how to use lambdas.
// Q must be a queue where the items are of type QItem
template<typename PUC, typename Q, typename QItem>
class PUCQueue {
private:
    static const int MAX_THREADS = 128;
    const int maxThreads;
    PUC puc{ PUC::alloc<Q>(), maxThreads };

public:
    PUCQueue(const int maxThreads=MAX_THREADS) : maxThreads{maxThreads} { }

    static std::string className() { return PUC::className() + Q::className(); }

    bool enqueue(QItem* item) {
        std::function<bool(Q*)> func = [item] (Q* q) { return q->enqueue(item); };
        return puc.updateTx(func);
    }

    QItem* dequeue(void) {
        std::function<QItem*(Q*)> func = [] (Q* q) { return q->dequeue(); };
        return (QItem*)puc.updateTx(func);
    }
};

#endif /* _PERSISTENT_UNIVERSAL_CONSTRUCT_QUEUE_H_ */
