#ifndef _TM_RESIZABLE_HASHByVal_MAP_H_
#define _TM_RESIZABLE_HASHByVal_MAP_H_

#include <string>

#include "ptmdb.h"
#include "slice.h"

namespace ptmdb {

/**
 * <h1> A Resizable Hash Map for usage in CX-Redo </h1>
 *
 */
class TMHashMap {
public:
    struct Node {
        PSlice              key;  // Immutable after node creation, so no need for persist<PSlice>
        PSlice              val;  // Mutates only when put() replaces the key
        TM_TYPE<Node*>      next {nullptr};
        Node(const Slice& k, const Slice& v):key{k},val{v}{}
    };


    TM_TYPE<long>                         capacity;
    TM_TYPE<long>                         sizeHM = 0;
    TM_TYPE<double>                       loadFactor = 2;
    alignas(128) TM_TYPE<TM_TYPE<Node*>*> buckets;      // An array of pointers to Nodes


public:
    TMHashMap(int capacity=10*1024*1024) : capacity{capacity} {
#ifdef PTMDB_CAPTURE_BY_COPY
        PTM_UPDATE_TX<bool>([=] () {
            buckets = (TM_TYPE<Node*>*)TM_PMALLOC(capacity*sizeof(TM_TYPE<Node*>));
            for (int i = 0; i < capacity; i++) buckets[i]=nullptr;
            return true;
        });
#else
        PTM_UPDATE_TX([&] () {
            buckets = (TM_TYPE<Node*>*)TM_PMALLOC(capacity*sizeof(TM_TYPE<Node*>));
            for (int i = 0; i < capacity; i++) buckets[i]=nullptr;
        });
#endif
    }


    ~TMHashMap() {
#ifdef PTMDB_CAPTURE_BY_COPY
        PTM_UPDATE_TX<bool>([=] () {
            for(int i = 0; i < capacity; i++){
                Node* node = buckets[i];
                while (node!=nullptr) {
                    Node* next = node->next;
                    PTM_DELETE(node);
                    node = next;
                }
            }
            TM_PFREE(buckets);
            return true;
        });
#else
        PTM_UPDATE_TX([&] () {
            for(int i = 0; i < capacity; i++){
                Node* node = buckets[i];
                while (node!=nullptr) {
                    Node* next = node->next;
                    PTM_DELETE(node);
                    node = next;
                }
            }
            TM_PFREE(buckets);
        });
#endif
    }


    std::string className() { return TM_NAME() + "-HashMap"; }


    void rebuild() {
        int newcapacity = 2*capacity;
        printf("Extending Hashmap to %d buckets\n", newcapacity);
        TM_TYPE<Node*>* newbuckets = (TM_TYPE<Node*>*)TM_PMALLOC(newcapacity*sizeof(TM_TYPE<Node*>));
        assert(newbuckets != nullptr);
        for (int i = 0; i < newcapacity; i++) newbuckets[i] = nullptr;
        for (int i = 0; i < capacity; i++) {
            Node* node = buckets[i];
            while(node!=nullptr){
                Node* next = node->next;
                // TODO: humm we should use only has of Slice for simplicity
                auto h = std::hash<PSlice>{}(node->key) % newcapacity;
                node->next = newbuckets[h];
                newbuckets[h] = node;
                node = next;
            }
        }
        TM_PFREE(buckets);
        buckets = newbuckets;
        capacity = newcapacity;
    }


    /*
     * Adds a node with a key if the key is not present, otherwise replaces the value.
     * If saveOldValue is set, it will set 'oldValue' to the previous value, iff there was already a mapping.
     *
     * Returns true if there was no mapping for the key, false if there was already a value and it was replaced.
     */
    bool innerPut(const Slice& key, const Slice& value) {
        // TODO: re-enable hashmap rebuild
        //if (sizeHM > (long)(capacity.pload()*loadFactor)) rebuild();
        auto h = std::hash<Slice>{}(key) % capacity;
        Node* node = buckets[h];
        Node* prev = node;
        while (true) {
            if (node == nullptr) {
                Node* newnode = PTM_NEW<Node>(key,value);
                assert(newnode != nullptr);
                if (node == prev) {
                    buckets[h] = newnode;
                } else {
                    prev->next = newnode;
                }
                //sizeHM++;
                return true;  // New insertion
            }
            if (node->key == key) {
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
    bool innerRemove(const Slice& key) {
        auto h = std::hash<Slice>{}(key) % capacity;
        Node* node = buckets[h];
        Node* prev = node;
        while (true) {
            if (node == nullptr) return false;
            if (node->key == key) {
                if (node == prev) {
                    buckets[h] = node->next;
                } else {
                    prev->next = node->next;
                }
                //sizeHM--;
                PTM_DELETE(node);
                return true;
            }
            prev = node;
            node = node->next;
        }
    }


    /*
     * Returns true if key is present. Saves a copy of 'value' in 'oldValue' if 'saveOldValue' is set.
     */
    bool innerGet(const Slice& key, std::string* oldValue) {
        auto h = std::hash<Slice>{}(key) % capacity;
        Node* node = buckets[h];
        while (true) {
            if (node == nullptr) return false;
            if (node->key == key) {
                oldValue->assign(node->val.data(), node->val.size());  // Makes a copy of V
                return true;
            }
            node = node->next;
        }
    }
    
    int getBucket(const Slice& key) {
        return std::hash<Slice>{}(key) % capacity;
    }
};

} // end of namespace ptmdb

#endif /* _TM_RESIZABLE_HASH_MAP_H_ */
