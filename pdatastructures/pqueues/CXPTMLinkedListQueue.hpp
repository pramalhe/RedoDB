/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _CX_PTM_LINKED_LIST_QUEUE_H_
#define _CX_PTM_LINKED_LIST_QUEUE_H_

#include <stdexcept>

#include "ptms/cxptm/CXPTM.hpp"

/**
 * <h1> A Linked List queue using CX-PTM </h1>
 */
template<typename T>
class CXPTMLinkedListQueue {

private:
    struct Node {
        cx::persist<T> item;
        cx::persist<Node*> next {nullptr};
        Node(T userItem) : item{userItem} { }
    };

    alignas(128) cx::persist<Node*>  head {nullptr};
    alignas(128) cx::persist<Node*>  tail {nullptr};


public:
    T EMPTY {};

    CXPTMLinkedListQueue(unsigned int maxThreads=0) {
        cx::CX::updateTx<bool>([=] () {
            Node* sentinelNode = cx::CX::tmNew<Node>(EMPTY);
            head = sentinelNode;
            tail = sentinelNode;
            return true;
        });
    }


    ~CXPTMLinkedListQueue() {
        cx::CX::updateTx<bool>([=] () {
            while (dequeue() != EMPTY); // Drain the queue
            Node* lhead = head;
            cx::CX::tmDelete(lhead);
            return true;
        });
    }


    static std::string className() { return "CX-PTM-LinkedListQueue"; }


    /*
     * Progress Condition: wait-free
     * Always returns true
     */
    bool enqueue(T item, const int tid=0) {
        if (item == EMPTY) throw std::invalid_argument("item can not be nullptr");
        return cx::CX::updateTx<bool>([=] () {
            Node* newNode = cx::CX::tmNew<Node>(item);
            tail->next = newNode;
            tail = newNode;
            return true;
        });
    }


    /*
     * Progress Condition: wait-free
     */
    T dequeue(const int tid=0) {
        return cx::CX::updateTx<T>([=] () {
            T item = EMPTY;
            Node* lhead = head;
            if (lhead == tail) return EMPTY;
            head = lhead->next;
            cx::CX::tmDelete(lhead);
            item = head->item;
            return item;
        });
    }
};

#endif /* _PERSISTENT_ROMULUS_LR_LINKED_LIST_QUEUE_H_ */
