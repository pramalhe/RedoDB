/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _PTM_FIXED_SIZE_HASH_MAP_H_
#define _PTM_FIXED_SIZE_HASH_MAP_H_

#include <string>
#include <cassert>

/**
 * <h1> A fixed size Hash Map for usage with PTMs </h1>
 * TODO
 *
 */
template<typename K, typename V, typename TM, template <typename> class TMTYPE>
class TMHashMapFixedSize {

private:
    struct Node {
        TMTYPE<K>        key;
        TMTYPE<V>        val;
        TMTYPE<Node*>    next {nullptr};
        TMTYPE<uint64_t> isActive {1};
        Node(const K& k, const V& value) : key{k}, val{value} { }
    };



    alignas(128) TMTYPE<TMTYPE<Node*>*> buckets;      // An array of pointers to Nodes
public:
    TMTYPE<uint64_t>                        capacity;

    // The default size is hard-coded to 2048 entries in the buckets array
    TMHashMapFixedSize(int capa=2048) {
        TM::template updateTx<bool>([=] () {
            capacity = capa;
            buckets = (TMTYPE<Node*>*)TM::pmalloc(capacity*sizeof(TMTYPE<Node*>));
            for (int i = 0; i < capacity; i++) buckets[i] = nullptr;
            return true;
	    });
    }

    ~TMHashMapFixedSize() {
        TM::template updateTx<bool>([=] () {
            for(int i = 0; i < capacity; i++){
                Node* node = buckets[i];
                while (node!=nullptr) {
                    Node* next = node->next;
                    TM::tmDelete(node);
                    node = next;
                }
            }
            TM::pfree(buckets);
            return true;
        });
    }


    static std::string className() { return TM::className() + "-HashMapFixedSize"; }


    /*
     * Adds a node with a key if the key is not present, otherwise replaces the value.
     * Returns the previous value (nullptr by default).
     */
    bool innerPut(const K& key, const V& value, V& oldValue, const bool saveOldValue) {
        auto h = std::hash<K>{}(key) % capacity;
        Node* node = buckets[h];
        Node* prev = node;
        Node* replnode = nullptr;
        V* oldVal = nullptr;
        while (true) {
            if (node == nullptr) {
                if (replnode != nullptr) {
                    // Found a replacement node
                    assert(replnode->isActive.pload() == 0);
                    replnode->key = key;
                    replnode->val = value;
                    replnode->isActive = 1;
                    return true;
                }
                Node* newnode = TM::template tmNew<Node>(key,value);
                if (node == prev) {
                    buckets[h] = newnode;
                } else {
                    prev->next = newnode;
                }
                return true;
            }
            if (key == node->key && node->isActive.pload() == 1) {
                if (saveOldValue) oldValue = node->val; // Makes a copy of V
                node->val = value;
                return false; // Replace value for existing key
            }
            if (replnode == nullptr && node->isActive.pload() == 0) replnode = node;
            prev = node;
            node = node->next;
        }
    }

    /*
     * Removes a key, returning the value associated with it.
     * Returns nullptr if there is no matching key.
     */
    bool innerRemove(const K& key, V& oldValue, const bool saveOldValue) {
        auto h = std::hash<K>{}(key) % capacity;
        Node* node = buckets[h];
        while (true) {
            if (node == nullptr) return false;
            if (key == node->key && node->isActive.pload() == 1) {
                if (saveOldValue) oldValue = node->val; // Makes a copy of V
                node->isActive = 0; // Instead of tmDelete()
                return true;
            }
            node = node->next;
        }
    }


    /*
     * Returns the value associated with the key, nullptr if there is no mapping
     */
    bool innerGet(const K& key, V& oldValue, const bool saveOldValue) {
        auto h = std::hash<K>{}(key) % capacity;
        Node* node = buckets[h];
        while (true) {
            if (node == nullptr) return false;
            if (key == node->key && node->isActive.pload() == 1) {
                if (saveOldValue) oldValue = node->val; // Makes a copy of V
                return true;
            }
            node = node->next;
        }
    }


    //
    // Set methods for running the usual tests and benchmarks
    //

    bool add(const K& key) {
        return TM::template updateTx<bool>([=] () {
            V notused;
            return innerPut(key,key,notused,false);
        });
    }

    bool add(const K& key, const V& value) {
        return TM::template updateTx<bool>([=] () {
            V notused;
            return innerPut(key,value,notused,false);
        });
    }

    bool remove(const K& key) {
        return TM::template updateTx<bool>([=] () {
            V notused;
            return innerRemove(key,notused,false);
        });
    }

    bool contains(const K& key) {
        return TM::template readTx<bool>([=] () {
            V notused;
            return innerGet(key,notused,false);
        });
    }

    // Used only for benchmarks
    bool addAll(K** keys, const int size) {
        for (int i = 0; i < size; i++) add(*keys[i]);
        return true;
    }
};

#endif /* _PTM_FIXED_SIZE_HASH_MAP_H_ */
