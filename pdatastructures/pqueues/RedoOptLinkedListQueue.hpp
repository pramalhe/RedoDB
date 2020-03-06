/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _REDOOPT_LINKED_LIST_QUEUE_H_
#define _REDOOPT_LINKED_LIST_QUEUE_H_

#include <stdexcept>

#include "ptms/redoopt/RedoOpt.hpp"

/**
 * <h1> A Linked List queue using Redo-Timed-Hash </h1>
 */
template<typename T>
class RedoOptLinkedListQueue {

private:
    struct Node {
        redoopt::persist<T> item;
        redoopt::persist<Node*> next {nullptr};
        Node(T userItem) : item{userItem} { }
    };

    redoopt::persist<Node*>  head {nullptr};
    redoopt::persist<Node*>  tail {nullptr};


public:
    T EMPTY {};

    RedoOptLinkedListQueue(unsigned int maxThreads=0) {
        redoopt::RedoOpt::updateTx<bool>([=] () {
            Node* sentinelNode = redoopt::RedoOpt::tmNew<Node>(EMPTY);
            head = sentinelNode;
            tail = sentinelNode;
            return true;
        });
    }


    ~RedoOptLinkedListQueue() {
        redoopt::RedoOpt::updateTx<bool>([=] () {
            while (dequeue() != EMPTY); // Drain the queue
            Node* lhead = head;
            redoopt::RedoOpt::tmDelete(lhead);
            return true;
        });
    }


    static std::string className() { return "RedoOpt-LinkedListQueue"; }


    /*
     * Progress Condition: wait-free
     * Always returns true
     */
    bool enqueue(T item, const int tid=0) {
        if (item == EMPTY) throw std::invalid_argument("item can not be nullptr");
        return redoopt::RedoOpt::updateTx<bool>([=] () {
            Node* newNode = redoopt::RedoOpt::tmNew<Node>(item);
            tail->next = newNode;
            tail = newNode;
            return true;
        });
    }


    /*
     * Progress Condition: wait-free
     */
    T dequeue(const int tid=0) {
        return redoopt::RedoOpt::updateTx<T>([=] () {
            T item = EMPTY;
            Node* lhead = head;
            if (lhead == tail) return EMPTY;
            head = lhead->next;
            redoopt::RedoOpt::tmDelete(lhead);
            item = head->item;
            return item;
        });
    }
};

#endif /* _REDOOPT_LINKED_LIST_QUEUE_H_ */
