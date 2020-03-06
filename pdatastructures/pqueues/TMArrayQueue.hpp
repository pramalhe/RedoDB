/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _TM_ARRAY_QUEUE_H_
#define _TM_ARRAY_QUEUE_H_

#include <string>


/**
 * <h1> An array-backed queue (memory bounded) for usage with STMs and PTMs </h1>
 * SIZE should be a power of 2 (for modulus to be faster).
 * <T> must be sizeof(int64_t). If it's not, then use a pointer to your type (T*).
 *
 */
template<typename K, int SIZE, typename TM, template <typename> class TMTYPE>
class TMArrayQueue {

private:
    TMTYPE<int64_t> head {0};
    TMTYPE<int64_t> tail {0};
    TMTYPE<K>       items[SIZE];

public:
    K EMPTY {};

    TMArrayQueue() { }

    ~TMArrayQueue() { }

    static std::string className() { return TM::className() + "-ArrayQueue"; }

    // Returns false if queue is full
    bool enqueue(K item, const int tid=0) {
        return TM::template updateTx<bool>([=] () {
            if (head + SIZE == tail) return false;
            items[tail%SIZE] = item;
            tail++;
            return true;
        });
    }

    // Returns EMPTY if queue is empty
    K dequeue(const int tid=0) {
        return TM::template updateTx<K>([=] () -> K {
            if (head == tail) return EMPTY;
            K item = items[head%SIZE];
            head++;
            return item;
        });
    }
};

#endif /* _TM_ARRAY_QUEUE_H_ */
