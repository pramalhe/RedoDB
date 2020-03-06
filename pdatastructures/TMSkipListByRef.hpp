/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 *
 * SkipList set (C++)
 * Original source code from https://www.sanfoundry.com/cpp-program-implement-skip-list/
 */
#ifndef _TM_SKIPLIST_BY_REF_H_
#define _TM_SKIPLIST_BY_REF_H_

#pragma once

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>

#define SK_MAX_LEVEL_BY_REF   23

template <typename E, typename TM, template <typename> class TMTYPE>
class TMSkipListByRef {

    struct SNode {
        /*-- Fields --*/
        TMTYPE<E>      key;
        TMTYPE<SNode*> forw[SK_MAX_LEVEL+1];

        SNode(int level, E k){
            for(int i=0;i<level + 1;i++) forw[i] = nullptr;
            key = k;
        }
    };


	/*---- Fields ----*/

    TMTYPE<SNode*>  header;
    TMTYPE<int64_t> level;



	/*---- Constructors ----*/

	// The degree is the minimum number of children each non-root internal node must have.
	public: TMSkipListByRef() {
	    TM::template updateTx([&] () {
		    header = TM::template tmNew<SNode>(SK_MAX_LEVEL, 0);
		    level = 0;
	    });
	}

    ~TMSkipListByRef() {
        TM::template updateTx([&] () {
            TM::template tmDelete<SNode>(header);
        });
    }


	/*---- Methods ----*/

    /* Random Value Generator */
    float frand() {
        return (float) rand() / RAND_MAX;
    }

    /* Random Level Generator */
    int random_level() {
        static bool first = true;
        if (first){
            srand((unsigned)time(nullptr));
            first = false;
        }

        int lvl = (int)(log(frand()) / log(1.-0.5f));
        return lvl < SK_MAX_LEVEL ? lvl : SK_MAX_LEVEL;
    }

    public: void display(){
        const SNode* x = header->forw[0];
        while (x != nullptr){
            std::cout << x->key;
            x = x->forw[0];
            if (x != nullptr)
                std::cout << " - ";
        }
        std::cout << std::endl;
    }

	public: bool contains(E key, const int tid=0) const {
	    bool retval = false;
	    TM::template readTx([&] () {
            SNode* x = header;
            for (int i = level;i >= 0;i--){
                while (x->forw[i] != nullptr && x->forw[i].pload()->key < key){
                    x = x->forw[i];
                }
            }
            x = x->forw[0];
            retval = (x != nullptr && x->key == key);
	    });
	    return retval;
	}


	public: bool add(E key, const int tid=0) {
	    bool retval = false;
        TM::template updateTx([&] () {
            SNode* x = header;
            SNode* update[SK_MAX_LEVEL + 1];
            for(int i=0;i<SK_MAX_LEVEL + 1;i++){
                update[i] = nullptr;
            }

            for (int i = level;i >= 0;i--){
                while (x->forw[i] != nullptr && x->forw[i]->key < key){
                    x = x->forw[i];
                }
                update[i] = x;
            }

            x = x->forw[0];

            if (x == nullptr || x->key != key){
                int lvl = random_level();
                if (lvl > level) {
                    for (int i = level + 1;i <= lvl;i++){
                        update[i] = header;
                    }
                    level = lvl;
                }
                x = TM::template tmNew<SNode>(lvl, key);
                for (int i = 0;i <= lvl;i++){
                    x->forw[i] = update[i]->forw[i];
                    update[i]->forw[i] = x;
                }
                retval = true;
                return;
            }
            retval = false;
        });
        return retval;
	}


	public: bool remove(E key, const int tid=0) {
	    bool retval = false;
        TM::template updateTx([&] () {
            SNode* x = header;
            SNode* update[SK_MAX_LEVEL + 1];
            for(int i=0;i<SK_MAX_LEVEL + 1;i++){
                update[i] = nullptr;
            }

            for (int i = level;i >= 0;i--){
                while (x->forw[i] != nullptr && x->forw[i].pload()->key < key){
                    x = x->forw[i];
                }
                update[i] = x;
            }

            x = x->forw[0];

            if (x != nullptr && x->key == key){
                for (int i = 0;i <= level;i++){
                    if (update[i]->forw[i] != x) break;
                    update[i]->forw[i] = x->forw[i];
                }
                TM::template tmDelete<SNode>(x);
                while (level.pload() > 0 && header->forw[level] == nullptr){
                    level--;
                }
                retval = true;
                return;
            }
            retval = false;
        });
        return retval;
	}

public:

    void addAll(E** keys, uint64_t size, const int tid=0) {
        for (uint64_t i = 0; i < size; i++) add(*keys[i], tid);
    }

    static std::string className() { return TM::className() + "-SkipList"; }

};
#endif   // _SEQUENTIAL_SKIPLIST_H_
