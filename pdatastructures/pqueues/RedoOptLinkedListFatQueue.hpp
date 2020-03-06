/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _REDOOPT_LINKED_LIST_FAT_QUEUE_H_
#define _REDOOPT_LINKED_LIST_FAT_QUEUE_H_

#include <stdexcept>

#include "ptms/redoopt/RedoOpt.hpp"

/**
 * <h1> A Linked List queue with fat nodes using Redo-Timed-Hash </h1>
 */
template<typename T>
class RedoOptLinkedListFatQueue {
    static const uint64_t NUM_ITEMS = 16-3; // also tried with 8-3 but not as good performance on optane

private:
    struct Node {
        redoopt::persist<uint64_t> it {1};
        redoopt::persist<uint64_t> ih {0};
        redoopt::persist<T>        item[NUM_ITEMS];
        redoopt::persist<Node*>    next {nullptr};
        Node(T userItem) {
            item[0] = userItem;
        }
    };

    alignas(128) redoopt::persist<Node*>  head {nullptr};
    alignas(128) redoopt::persist<Node*>  tail {nullptr};


public:
    T EMPTY {};

    RedoOptLinkedListFatQueue(unsigned int maxThreads=0) {
        redoopt::RedoOpt::updateTx<bool>([=] () {
            Node* sentinelNode = redoopt::RedoOpt::tmNew<Node>(EMPTY);
            sentinelNode->it = NUM_ITEMS;
            sentinelNode->ih = NUM_ITEMS;
            head = sentinelNode;
            tail = sentinelNode;
            return true;
        });
    }


    ~RedoOptLinkedListFatQueue() {
        redoopt::RedoOpt::updateTx<bool>([=] () {
            while (dequeue() != EMPTY); // Drain the queue
            Node* lhead = head;
            redoopt::RedoOpt::tmDelete(lhead);
            return true;
        });
    }


    static std::string className() { return "RedoOpt-LinkedListFatQueue"; }


    /*
     * Progress Condition: wait-free
     * Always returns true
     */
    bool enqueue(T item, const int tid=0) {
        if (item == EMPTY) throw std::invalid_argument("item can not be nullptr");
        return redoopt::RedoOpt::updateTx<bool>([=] () {
            Node* ltail = tail;
            if (ltail->it == NUM_ITEMS) {
                Node* newNode = redoopt::RedoOpt::tmNew<Node>(item);
                tail->next = newNode;
                tail = newNode;
            } else {
                ltail->item[ltail->it] = item;
                ltail->it++;
            }
            return true;
        });
    }


    /*
     * Progress Condition: wait-free
     */
    T dequeue(const int tid=0) {
        return redoopt::RedoOpt::updateTx<T>([=] () {
            Node* lhead = head;
            if (lhead == tail) return EMPTY;
            if (lhead->ih == NUM_ITEMS) {
                head = lhead->next;
                redoopt::RedoOpt::tmDelete(lhead);
                lhead = head;
            }
            T item = lhead->item[lhead->ih];
            lhead->ih++;
            return item;
        });
    }
};

#endif /* _REDOOPT_LINKED_LIST_QUEUE_H_ */
