/*
 * Copyright 2018-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _VOLATILE_TO_PERSISTENT_MAP_H_
#define _VOLATILE_TO_PERSISTENT_MAP_H_



/**
 * <h1> This is a volatile wrapper to a persistent map</h1>
 *
 * We've created this for greater comodity to the user.
 * No need to place this inside a transaction, just instanciate it with 'new'
 * and use it like any other data structure in volatile memory.
 */
template<typename K, typename V, typename PTM, typename PMAP>
class V2PMap {
private:
    int    _objidx;

public:
    V2PMap(int idx=0) {
        _objidx = idx;
        PTM::template updateTx<PMAP*>([=] () {
            PMAP* pmap = PTM::template tmNew<PMAP>();
            PTM::template put_object<PMAP>(idx, pmap);
            return pmap;
        });
    }

    ~V2PMap() {
        int idx = _objidx;
        PTM::template updateTx<bool>([=] () {
            PMAP* pmap = PTM::template get_object<PMAP>(idx);
            PTM::tmDelete(pmap);
            PTM::template put_object<PMAP>(idx, nullptr);
            return true;
        });
    }

    static std::string className() { return PMAP::className(); }

    //
    // Set methods for running the usual tests and benchmarks
    //

    // Inserts a key/value
    bool put(K key, V value) {
        int idx = _objidx;
        return PTM::template updateTx<bool>([=] () {
            PMAP* pmap = PTM::template get_object<PMAP>(idx);
            return pmap->innerPut(key, value);
        });
    }

    // Returns true only if the key was present
    bool remove(K key) {
        int idx = _objidx;
        return PTM::template updateTx<bool>([=] () {
            PMAP* pmap = PTM::template get_object<PMAP>(idx);
            return pmap->remove(key);
        });
    }

    // Returns true if the key is present in the set
    bool contains(K key) {
        int idx = _objidx;
        return PTM::template readTx<bool>([=] () {
            PMAP* pmap = PTM::template get_object<PMAP>(idx);
            return pmap->contains(key);
        });
    }

    // Returns the value matching a key. V must be a pointer or 64bits, otherwise we need to implement WFObject
    V getValue(K key) {
        int idx = _objidx;
        return PTM::template readTx<V>([=] () {
            PMAP* pmap = PTM::template get_object<PMAP>(idx);
            return pmap->getValue(key);
        });
    }

    // Used only for benchmarks
    bool addAll(K** keys, const int size) {
        for (int i = 0; i < size; i++) add(*keys[i]);
        return true;
    }
};

#endif /* _VOLATILE_TO_PERSISTENT_SET_H_ */
