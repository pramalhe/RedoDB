/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _REDO_TIMED_LINKED_LIST_QUEUE_H_
#define _REDO_TIMED_LINKED_LIST_QUEUE_H_

#include <stdexcept>

#include "ptms/redotimed/RedoTimed.hpp"

/**
 * <h1> A Linked List queue using CX-Redo-Timed </h1>
 */
template<typename T>
class RedoTimedLinkedListQueue {

private:
    struct Node {
        redotimed::persist<T> item;
        redotimed::persist<Node*> next {nullptr};
        Node(T userItem) : item{userItem} { }
    };

    alignas(128) redotimed::persist<Node*>  head {nullptr};
    alignas(128) redotimed::persist<Node*>  tail {nullptr};


public:
    T EMPTY {};

    RedoTimedLinkedListQueue(unsigned int maxThreads=0) {
        redotimed::RedoTimed::updateTx<bool>([=] () {
            Node* sentinelNode = redotimed::RedoTimed::tmNew<Node>(EMPTY);
            head = sentinelNode;
            tail = sentinelNode;
            return true;
        });
    }


    ~RedoTimedLinkedListQueue() {
        redotimed::RedoTimed::updateTx<bool>([=] () {
            while (dequeue() != EMPTY); // Drain the queue
            Node* lhead = head;
            redotimed::RedoTimed::tmDelete(lhead);
            return true;
        });
    }


    static std::string className() { return "RedoTimed-LinkedListQueue"; }


    /*
     * Progress Condition: wait-free
     * Always returns true
     */
    bool enqueue(T item, const int tid=0) {
        if (item == EMPTY) throw std::invalid_argument("item can not be nullptr");
        return redotimed::RedoTimed::updateTx<bool>([=] () {
            Node* newNode = redotimed::RedoTimed::tmNew<Node>(item);
            tail->next = newNode;
            tail = newNode;
            return true;
        });
    }


    /*
     * Progress Condition: wait-free
     */
    T dequeue(const int tid=0) {
        return redotimed::RedoTimed::updateTx<T>([=] () {
            T item = EMPTY;
            Node* lhead = head;
            if (lhead == tail) return EMPTY;
            head = lhead->next;
            redotimed::RedoTimed::tmDelete(lhead);
            item = head->item;
            return item;
        });
    }
};

#endif /* _REDO_TIMED_LINKED_LIST_QUEUE_H_ */
