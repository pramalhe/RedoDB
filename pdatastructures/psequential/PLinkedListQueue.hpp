/*
 * Copyright 2018-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _PERSISTENT_PUC_SEQUENTIAL_LINKED_LIST_QUEUE_H_
#define _PERSISTENT_PUC_SEQUENTIAL_LINKED_LIST_QUEUE_H_

/**
 * <h1> A persistent sequential implementation of a Linked List Queue </h1>
 *
 * This is meant to be used by Persistent Universal Constructs
 */
template<typename T, typename TM>
class PLinkedListQueue {

private:
    struct Node {
        T item;
        Node* next {nullptr};
        Node(T userItem) : item{userItem} { }
    };

    Node*  head {nullptr};
    Node*  tail {nullptr};

public:
    T EMPTY {};

    PLinkedListQueue(unsigned int maxThreads=0) {
        Node* sentinelNode = TM::template tmNew<Node>(EMPTY);
        head = sentinelNode;
        tail = sentinelNode;
    }

    // Universal Constructs need a copy constructor on the underlying data structure
    PLinkedListQueue(const PLinkedListQueue& other) {
        head = TM::template tmNew<Node>(EMPTY);
        Node* node = head;
        Node* onode = other.head->next;
        while (onode != nullptr) {
            node->next = TM::template tmNew<Node>(onode->item);
            node = node->next;
            onode = onode->next;
        }
        tail = node;
    }

    ~PLinkedListQueue() {
        while (dequeue() != EMPTY); // Drain the queue
        Node* lhead = head;
        TM::tmDelete(lhead);
    }

    static std::string className() { return "PLinkedListQueue"; }

    bool enqueue(T item) {
        if (item == EMPTY) throw std::invalid_argument("item can not be nullptr");
        Node* newNode = TM::template tmNew<Node>(item);
        tail->next = newNode;
        tail = newNode;
        return true;
    }

    T dequeue() {
        Node* lhead = head;
        if (lhead == tail) return EMPTY;
        head = lhead->next;
        TM::tmDelete(lhead);
        return head->item;
    }
};

#endif /* _PERSISTENT_SEQUENTIAL_LINKED_LIST_QUEUE_H_ */
