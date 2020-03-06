/*
 * Copyright 2017-2018
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *   Nachshon Cohen <nachshonc@gmail.com>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _PERSISTENT_OF_WF_LINKED_LIST_QUEUE_H_
#define _PERSISTENT_OF_WF_LINKED_LIST_QUEUE_H_

#include <stdexcept>

#include "ptms/ponefilewf/OneFilePTMWF.hpp"

/**
 * <h1> A Linked List queue using OneFile PTM (Wait-Free) </h1>
 *
 * enqueue algorithm: sequential implementation + OF-WF
 * dequeue algorithm: sequential implementation + OF-WF
 * Consistency: Linearizable
 * enqueue() progress: wait-free
 * dequeue() progress: wait-free
 * enqueue min ops: 2 DCAS + 1 CAS
 * dequeue min ops: 1 DCAS + 1 CAS
 */
template<typename T>
class POFWFLinkedListQueue : public onefileptmwf::tmbase {

private:
    struct Node : onefileptmwf::tmbase {
        onefileptmwf::tmtype<T> item;
        onefileptmwf::tmtype<Node*> next {nullptr};
        //Node(T* userItem) : item{userItem} { }
    };

    onefileptmwf::tmtype<Node*>  head {nullptr};
    onefileptmwf::tmtype<Node*>  tail {nullptr};


public:
    T EMPTY {};

    POFWFLinkedListQueue(unsigned int maxThreads=0) {
        onefileptmwf::updateTx([=] () {
            //Node* sentinelNode = onefileptmwf::tmNew<Node>(nullptr);
            Node* sentinelNode = onefileptmwf::tmNew<Node>();
            sentinelNode->item = EMPTY;
            sentinelNode->next = nullptr;
            head = sentinelNode;
            tail = sentinelNode;
        });
    }


    ~POFWFLinkedListQueue() {
        onefileptmwf::updateTx([=] () {
            while (dequeue() != EMPTY); // Drain the queue
            Node* lhead = head;
            onefileptmwf::tmDelete(lhead);
        });
    }


    static std::string className() { return "POF-WF-LinkedListQueue"; }


    /*
     * Progress Condition: wait-free
     * Always returns true
     */
    bool enqueue(T item, const int tid=0) {
        if (item == EMPTY) throw std::invalid_argument("item can not be nullptr");
        return onefileptmwf::updateTx<bool>([this,item] () -> bool {
            //Node* newNode = onefileptmwf::tmNew<Node>(item);
            Node* newNode = onefileptmwf::tmNew<Node>();
            newNode->item = item;
            newNode->next = nullptr;
            tail->next = newNode;
            tail = newNode;
            return true;
        });
    }


    /*
     * Progress Condition: wait-free
     */
    T dequeue(const int tid=0) {
        return onefileptmwf::updateTx<T>([this] () -> T {
            Node* lhead = head;
            if (lhead == tail) return EMPTY;
            head = lhead->next;
            onefileptmwf::tmDelete(lhead);
            return head->item;
        });
    }
};

#endif /* _PERSISTENT_OF_WF_LINKED_LIST_QUEUE_H_ */
