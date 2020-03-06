/*
 * Copyright 2018-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _VOLATILE_TO_PERSISTENT_MAP_BY_REF_H_
#define _VOLATILE_TO_PERSISTENT_MAP_BY_REF_H_



/**
 * <h1> This is a volatile wrapper to a persistent map</h1>
 *
 * We've created this for greater comodity to the user.
 * No need to place this inside a transaction, just instanciate it with 'new'
 * and use it like any other data structure in volatile memory.
 */
template<typename K, typename V, typename PTM, typename PMAP>
class V2PMapByRef {
private:
    int    _objidx;

public:
    V2PMapByRef(int idx=0) {
        _objidx = idx;
        PTM::updateTx([&] () {
            PMAP* pmap = PTM::template tmNew<PMAP>();
            PTM::template put_object<PMAP>(idx, pmap);
        });
    }

    ~V2PMapByRef() {
        int idx = _objidx;
        PTM::updateTx([&] () {
            PMAP* pmap = PTM::template get_object<PMAP>(idx);
            PTM::tmDelete(pmap);
            PTM::template put_object<PMAP>(idx, nullptr);
        });
    }

    static std::string className() { return PMAP::className(); }

    //
    // Set methods for running the usual tests and benchmarks
    //

    // Inserts a key/value
    bool put(K key, V value) {
        int idx = _objidx;
        bool retval = false;
        PTM::updateTx([&] () {
            PMAP* pmap = PTM::template get_object<PMAP>(idx);
            retval = pmap->innerPut(key, value);
        });
        return retval;
    }

    // Returns true only if the key was present
    bool remove(K key) {
        int idx = _objidx;
        bool retval = false;
        PTM::updateTx([&] () {
            PMAP* pmap = PTM::template get_object<PMAP>(idx);
            retval = pmap->remove(key);
        });
        return retval;
    }

    // Returns true if the key is present in the set
    bool contains(K key) {
        int idx = _objidx;
        bool retval = false;
        PTM::readTx([&] () {
            PMAP* pmap = PTM::template get_object<PMAP>(idx);
            retval = pmap->contains(key);
        });
        return retval;
    }

    // Returns the value matching a key. V must be a pointer or 64bits, otherwise we need to implement WFObject
    V getValue(K key) {
        int idx = _objidx;
        V retval {};
        PTM::readTx([&] () {
            PMAP* pmap = PTM::template get_object<PMAP>(idx);
            retval = pmap->getValue(key);
        });
        return retval;
    }

    // Used only for benchmarks
    bool addAll(K** keys, const int size) {
        for (int i = 0; i < size; i++) add(*keys[i]);
        return true;
    }
};

#endif /* _VOLATILE_TO_PERSISTENT_SET_H_ */
