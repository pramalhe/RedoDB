/*
 * Copyright 2018-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _PERSISTENT_UNIVERSAL_CONSTRUCT_QUEUE_WRAPPER_H_
#define _PERSISTENT_UNIVERSAL_CONSTRUCT_QUEUE_WRAPPER_H_

#include <atomic>
#include <stdexcept>
#include <cstdint>
#include <functional>
#include "CXPUC.hpp"


// This class can be used to simplify the usage of queues with Universal Constructs
// For generic code you don't need it (and can't use it), but it can serve as an example of how to use lambdas.
// Q must be a queue where the items are of type QItem
template<typename Q, typename T>
class PUCQueue {
private:
    cxpuc::CX<Q> puc {nullptr}; // Initializes with empty queue

public:
    T EMPTY {};

    PUCQueue() { }

    static std::string className() { return "CXPUC-" + Q::className(); }

    bool enqueue(T key, const int tid=0) {
        std::function<bool(Q*)> func = [key] (Q* q) { return q->enqueue(key); };
        return puc.template updateTx<bool>(func);
    }

    T dequeue(const int tid=0) {
        std::function<T(Q*)> func = [] (Q* q) { return q->dequeue(); };
        return puc.template updateTx<T>(func);
    }
};

#endif /* _UNIVERSAL_CONSTRUCT_QUEUE_H_ */
