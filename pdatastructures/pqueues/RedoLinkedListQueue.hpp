/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _REDO_LINKED_LIST_QUEUE_H_
#define _REDO_LINKED_LIST_QUEUE_H_

#include <stdexcept>

#include "ptms/redo/Redo.hpp"

/**
 * <h1> A Linked List queue using CX-Redo </h1>
 */
template<typename T>
class RedoLinkedListQueue {

private:
    struct Node {
        redo::persist<T> item;
        redo::persist<Node*> next {nullptr};
        Node(T userItem) : item{userItem} { }
    };

    alignas(128) redo::persist<Node*>  head {nullptr};
    alignas(128) redo::persist<Node*>  tail {nullptr};


public:
    T EMPTY {};

    RedoLinkedListQueue(unsigned int maxThreads=0) {
        redo::Redo::updateTx<bool>([=] () {
            Node* sentinelNode = redo::Redo::tmNew<Node>(EMPTY);
            head = sentinelNode;
            tail = sentinelNode;
            return true;
        });
    }


    ~RedoLinkedListQueue() {
        redo::Redo::updateTx<bool>([=] () {
            while (dequeue() != EMPTY); // Drain the queue
            Node* lhead = head;
            redo::Redo::tmDelete(lhead);
            return true;
        });
    }


    static std::string className() { return "Redo-LinkedListQueue"; }


    /*
     * Progress Condition: wait-free
     * Always returns true
     */
    bool enqueue(T item, const int tid=0) {
        if (item == EMPTY) throw std::invalid_argument("item can not be nullptr");
        return redo::Redo::updateTx<bool>([=] () {
            Node* newNode = redo::Redo::tmNew<Node>(item);
            tail->next = newNode;
            tail = newNode;
            return true;
        });
    }


    /*
     * Progress Condition: wait-free
     */
    T dequeue(const int tid=0) {
        return redo::Redo::updateTx<T>([=] () {
            T item = EMPTY;
            Node* lhead = head;
            if (lhead == tail) return EMPTY;
            head = lhead->next;
            redo::Redo::tmDelete(lhead);
            item = head->item;
            return item;
        });
    }
};

#endif /* _REDO_LINKED_LIST_QUEUE_H_ */
