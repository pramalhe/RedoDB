/*
 * Copyright 2018-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _VOLATILE_TO_PERSISTENT_SET_H_
#define _VOLATILE_TO_PERSISTENT_SET_H_



/**
 * <h1> This is a volatile wrapper to a persistent set</h1>
 *
 * We've created this for greater comodity to the user.
 * No need to place this inside a transaction, just instanciate it with 'new'
 * and use it like any other data structure in volatile memory.
 */
template<typename K, typename PTM, typename PSET>
class V2PSet {
private:
    int    _objidx;

public:
    V2PSet(int idx=0) {
        _objidx = idx;
        PTM::template updateTx<PSET*>([=] () {
            PSET* pset = PTM::template tmNew<PSET>();
            PTM::template put_object<PSET>(idx, pset);
            return pset;
        });
    }

    ~V2PSet() {
        int idx = _objidx;
        PTM::template updateTx<bool>([=] () {
            PSET* pset = PTM::template get_object<PSET>(idx);
            PTM::tmDelete(pset);
            PTM::template put_object<PSET>(idx, nullptr);
            return true;
        });
    }

    static std::string className() { return PSET::className(); }

    //
    // Set methods for running the usual tests and benchmarks
    //

    // Inserts a key only if it's not already present
    bool add(K key) {
        int idx = _objidx;
        return PTM::template updateTx<bool>([=] () {
            PSET* pset = PTM::template get_object<PSET>(idx);
            return pset->add(key);
        });
    }

    // Returns true only if the key was present
    bool remove(K key) {
        int idx = _objidx;
        return PTM::template updateTx<bool>([=] () {
            PSET* pset = PTM::template get_object<PSET>(idx);
            return pset->remove(key);
        });
    }

    // Returns true if the key is present in the set
    bool contains(K key) {
        int idx = _objidx;
        return PTM::template readTx<bool>([=] () {
            PSET* pset = PTM::template get_object<PSET>(idx);
            return pset->contains(key);
        });
    }

    // Used only for benchmarks
    bool addAll(K** keys, const int size) {
        for (int i = 0; i < size; i++) add(*keys[i]);
        return true;
    }
};

#endif /* _VOLATILE_TO_PERSISTENT_SET_H_ */
