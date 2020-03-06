/*
 * Copyright 2018-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _PERSISTENT_SEQUENTIAL_RESIZABLE_HASH_MAP_H_
#define _PERSISTENT_SEQUENTIAL_RESIZABLE_HASH_MAP_H_

#include <string>

#define HM_LOAD_FACTOR   0.75f

/**
 * <h1> A Resizable Hash Map for PTMs </h1>
 */
template<typename K, typename V, typename TM>
class PHashMap {

private:
    struct Node {
        K     key;
        V     val;
        Node* next {nullptr};
        Node(const K& k, const V& v) : key{k}, val{v} { } // Copy constructor for k and value
        Node(const Node& other) { key = other.key; val = other.val; }
    };


    uint64_t                    capacity;
    uint64_t                    sizeHM = 0;
    Node**                      buckets;      // An array of pointers to Nodes


public:
    PHashMap(uint64_t capacity=4) : capacity{capacity} {
        buckets = (Node**)TM::pmalloc(capacity*sizeof(Node*));
        for (int i = 0; i < capacity; i++) buckets[i] = nullptr;
    }


    // Universal Constructs need a copy constructor on the underlying data structure
    PHashMap(const PHashMap& other) {
        capacity = other.capacity;
        sizeHM = other.sizeHM;
        buckets = (Node**)TM::pmalloc(capacity*sizeof(Node*));
        for (int i = 0; i < capacity; i++) {
            if (other.buckets[i] == nullptr) {
                buckets[i] = nullptr;
                continue;
            }
            buckets[i] = TM::template tmNew<Node>(*other.buckets[i]);
            Node* node = buckets[i];
            Node* onode = other.buckets[i];
            while (onode->next != nullptr) {
                node->next = TM::template tmNew<Node>(*onode->next);
                node = node->next;
                onode = onode->next;
            }
        }
    }


    ~PHashMap() {
        for(int i = 0; i < capacity; i++){
            Node* node = buckets[i];
            while (node != nullptr) {
                Node* next = node->next;
                TM::template tmDelete(node);
                node = next;
            }
        }
        TM::pfree(buckets);
    }


    static std::string className() { return "PHashMap"; }


    void rebuild() {
        uint64_t newcapacity = 2*capacity;
        Node** newbuckets = (Node**)TM::pmalloc(newcapacity*sizeof(Node*));
        for (int i = 0; i < newcapacity; i++) newbuckets[i] = nullptr;
        for (int i = 0; i < capacity; i++) {
            Node* node = buckets[i];
            while (node != nullptr) {
                Node* next = node->next;
                auto h = std::hash<K>{}(node->key) % newcapacity;
                node->next = newbuckets[h];
                newbuckets[h] = node;
                node = next;
            }
        }
        TM::pfree(buckets);
        buckets = newbuckets;
        capacity = newcapacity;
    }


    /*
     * Adds a node with a key if the key is not present, otherwise replaces the value.
     * If saveOldValue is set, it will set 'oldValue' to the previous value, iff there was already a mapping.
     *
     * Returns true if there was no mapping for the key, false if there was already a value and it was replaced.
     */
    bool innerPut(const K& key, const V& value, V& oldValue, const bool saveOldValue) {
        if (sizeHM > capacity*HM_LOAD_FACTOR) rebuild();
        auto h = std::hash<K>{}(key) % capacity;
        Node* node = buckets[h];
        Node* prev = node;
        while (true) {
            if (node == nullptr) {
                Node* newnode = TM::template tmNew<Node>(key,value);
                if (node == prev) {
                    buckets[h] = newnode;
                } else {
                    prev->next = newnode;
                }
                sizeHM=sizeHM+1;
                return true;  // New insertion
            }
            if (key == node->key) {
                if (saveOldValue) oldValue = node->val; // Makes a copy of V
                node->val = value;
                return false; // Replace value for existing key
            }
            prev = node;
            node = node->next;
        }
    }


    /*
     * Removes a key and its mapping.
     * Saves the value in 'oldvalue' if 'saveOldValue' is set.
     *
     * Returns returns true if a matching key was found
     */
    bool innerRemove(const K& key, V& oldValue, const bool saveOldValue) {
        auto h = std::hash<K>{}(key) % capacity;
        Node* node = buckets[h];
        Node* prev = node;
        while (true) {
            if (node == nullptr) return false;
            if (key == node->key) {
                if (saveOldValue) oldValue = node->val; // Makes a copy of V
                if (node == prev) {
                    buckets[h] = node->next;
                } else {
                    prev->next = node->next;
                }
                sizeHM=sizeHM-1;
                TM::tmDelete(node);
                return true;
            }
            prev = node;
            node = node->next;
        }
    }


    /*
     * Returns true if key is present. Saves a copy of 'value' in 'oldValue' if 'saveOldValue' is set.
     */
    bool innerGet(const K& key, V& oldValue, const bool saveOldValue) {
        auto h = std::hash<K>{}(key) % capacity;
        Node* node = buckets[h];
        while (true) {
            if (node == nullptr) return false;
            if (key == node->key) {
                if (saveOldValue) oldValue = node->val; // Makes a copy of V
                return true;
            }
            node = node->next;
        }
    }


    //
    // Set methods for running the usual tests and benchmarks
    //

    // Inserts a key only if it's not already present
    bool add(K key) {
        V notused;
        return innerPut(key,key,notused,false);
    }

    // Returns true only if the key was present
    bool remove(K key) {
        V notused;
        return innerRemove(key,notused,false);
    }

    bool contains(K key) {
        V notused;
        return innerGet(key,notused,false);
    }

    // Used only for benchmarks
    bool addAll(K** keys, const int size) {
        for (int i = 0; i < size; i++) add(*keys[i]);
        return true;
    }
};

#endif /* _PERSISTENT_PERSISTENT_RESIZABLE_HASH_MAP_H_ */
