/*
 * Copyright 2018-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _PERSISTENT_UNIVERSAL_CONSTRUCT_SET_H_
#define _PERSISTENT_UNIVERSAL_CONSTRUCT_SET_H_

#include <functional>
#include "CXPUC.hpp"

/**
 * <h1> Interface for Persistent Universal Constructs (Sets) </h1>
 *
 * PSET is the set class
 * K is the type of the key of the set
 */

// This class can be used to simplify the usage of sets/multisets with CXPUC
// For generic code you don't need it (and can't use it), but it can serve as an example of how to use lambdas.
// PSET must be a set/multiset where the keys are of type K
template<typename PSET, typename K>
class PUCSet {
private:
    cxpuc::CX<PSET> puc {nullptr}; // Initializes with empty set

public:
    PUCSet() { }

    static std::string className() { return "CXPUC-" + PSET::className(); }

    bool add(K key) {
        std::function<bool(PSET*)> addFunc = [key] (PSET* set) { return set->add(key); };
        return puc.template updateTx<bool>(addFunc);
    }

    bool remove(K key) {
        std::function<bool(PSET*)> removeFunc = [key] (PSET* set) { return set->remove(key); };
        return puc.template updateTx<bool>(removeFunc);
    }

    bool contains(K key) {
        std::function<bool(PSET*)> containsFunc = [key] (PSET* set) { return set->contains(key); };
        return puc.template readTx<bool>(containsFunc);
    }

    void addAll(K** keys, const int size) {
        for (int i = 0; i < size; i++) add(*keys[i]);
    }
};

#endif /* _PERSISTENT_UNIVERSAL_CONSTRUCT_SET_H_ */
