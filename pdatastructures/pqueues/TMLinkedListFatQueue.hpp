/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _TM_LINKED_LIST_FAT_QUEUE_H_
#define _TM_LINKED_LIST_FAT_QUEUE_H_

#include <stdexcept>

/**
 * <h1> A Linked List queue with fat nodes using a PTM or STM </h1>
 * <T> must be sizeof(int64_t). If it's not, then use a pointer to your type (T*).
 */
template<typename T, typename TM, template <typename> class TMTYPE>
class TMLinkedListFatQueue {
    static const uint64_t NUM_ITEMS = 16-3; // also tried with 8-3 but not as good performance on optane

private:
    struct Node {
        TMTYPE<uint64_t> it {1};
        TMTYPE<uint64_t> ih {0};
        TMTYPE<T>        item[NUM_ITEMS];
        TMTYPE<Node*>    next {nullptr};
        Node(T userItem) {
            item[0] = userItem;
        }
    };

    TMTYPE<Node*>  head {nullptr};
    TMTYPE<Node*>  tail {nullptr};


public:
    T EMPTY {};

    TMLinkedListFatQueue() {
        TM::updateTx<bool>([=] () {
            Node* sentinelNode = TM::tmNew<Node>(EMPTY);
            sentinelNode->it = NUM_ITEMS;
            sentinelNode->ih = NUM_ITEMS;
            head = sentinelNode;
            tail = sentinelNode;
            return true;
        });
    }


    ~TMLinkedListFatQueue() {
        TM::updateTx<bool>([=] () {
            while (dequeue() != EMPTY); // Drain the queue
            Node* lhead = head;
            TM::tmDelete(lhead);
            return true;
        });
    }


    static std::string className() { return TM::className() + "-LinkedListFatQueue"; }

    /*
     * Always returns true
     */
    bool enqueue(T item, const int tid=0) {
        if (item == EMPTY) throw std::invalid_argument("item can not be nullptr");
        return TM::updateTx<bool>([=] () {
            Node* ltail = tail;
            if (ltail->it == NUM_ITEMS) {
                Node* newNode = TM::tmNew<Node>(item);
                tail->next = newNode;
                tail = newNode;
            } else {
                ltail->item[ltail->it] = item;
                ltail->it++;
            }
            return true;
        });
    }

    T dequeue(const int tid=0) {
        return TM::updateTx<T>([=] () {
            Node* lhead = head;
            if (lhead == tail) return EMPTY;
            if (lhead->ih == NUM_ITEMS) {
                head = lhead->next;
                TM::tmDelete(lhead);
                lhead = head;
            }
            T item = lhead->item[lhead->ih];
            lhead->ih++;
            return item;
        });
    }
};

#endif /* _TM_LINKED_LIST_FAT_QUEUE_H_ */
