/*
 * Copyright 2014-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _CIRCULARARRAY_H_
#define _CIRCULARARRAY_H_

#include <vector>
#include <iostream>

#include "../cxptm/HazardPointersCX.hpp"


/**
 * This is storing the pointers to the T instances, not the actual T instances.
 */
template<typename TNode, class HP = HazardPointersCX<TNode>>
class CircularArray {

private:
    static const int max_size = 2000;
    int min_size = 1000;
    TNode** preRetiredMutNodes;
    int begin = 0;
    int size = 0;
    HP& hp;
    int tid;

    void clean(TNode* node) {
        int pos = begin;
        int initialSize = size;
        TNode* lnext = nullptr;
        for(int i = 0;i<initialSize;i++){
            if(pos==max_size)pos=0;
            TNode* mNode = preRetiredMutNodes[pos];
            if(mNode->ticket.load() > node->ticket.load()-min_size) {
                begin = pos;
                return;
            }
            lnext = mNode->next.load();
            mNode->next.store(mNode, std::memory_order_release);
            hp.retire(lnext,tid);
            pos++;
            size--;
        }
    }


public:
    CircularArray(HP& hp, int tid):hp{hp},tid{tid} {
        preRetiredMutNodes = new TNode*[max_size];
        static_assert(std::is_same<decltype(TNode::ticket), std::atomic<uint64_t>>::value, "TNode::ticket must exist");
        static_assert(std::is_same<decltype(TNode::next), std::atomic<TNode*>>::value, "TNode::next must exist");
    }

    ~CircularArray() {
        int pos = begin;
        for(int i = 0;i<size;i++){
            if (pos == max_size) pos = 0;
            hp.retire(preRetiredMutNodes[pos]->next.load(),tid);
            pos++;
        }
        delete[] preRetiredMutNodes;
    }


    bool add(TNode* node) {
        if (size == max_size) clean(node);
        int pos = (begin+size)%max_size;
        preRetiredMutNodes[pos] = node;
        size++;
        return true;
    }
};

#endif /* _CIRCULARARRAY_H_ */
