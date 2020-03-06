/*
 * Copyright 2018-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _PERSISTENT_SEQUENTIAL_LINKED_LIST_SET_H_
#define _PERSISTENT SEQUENTIAL_LINKED_LIST_SET_H_

#include <string>

/**
 * <h1> A persistent sequential implementation of a Linked List Set </h1>
 *
 * This is meant to be used by Persistent Universal Constructs
 *
 */
template<typename K, typename TM>
class PLinkedListSet {

private:
    struct Node {
        K     key;
        Node* next{nullptr};
        Node(const K& key) : key{key}, next{nullptr} { }
        Node(){ }
    };

    Node*  head {nullptr};
    Node*  tail {nullptr};

public:
    PLinkedListSet() {
		Node* lhead = TM::template tmNew<Node>();
		Node* ltail = TM::template tmNew<Node>();
		head = lhead;
		head->next = ltail;
		tail = ltail;
    }

    // Universal Constructs need a copy constructor on the underlying data structure
    PLinkedListSet(const PLinkedListSet& other) {
        head = TM::template tmNew<Node>();
        Node* node = head;
        Node* onode = other.head->next;
        while (onode != other.tail) {
            node->next = TM::template tmNew<Node>(onode->key);
            node = node->next;
            onode = onode->next;
        }
        tail = TM::template tmNew<Node>();
        node->next = tail;
    }

    ~PLinkedListSet() {
		// Delete all the nodes in the list
		Node* prev = head;
		Node* node = prev->next;
		while (node != tail) {
			TM::tmDelete(prev);
			prev = node;
			node = node->next;
		}
		TM::tmDelete(prev);
		TM::tmDelete(tail);
    }

    static std::string className() { return "PLinkedListSet"; }

    /*
     * Adds a node with a key, returns false if the key is already in the set
     */
    bool add(const K& key) {
        Node *prev, *node;
        find(key, prev, node);
        bool retval = !(node != tail && key == node->key);
        if (!retval) return retval;
        Node* newNode = TM::template tmNew<Node>(key);
        prev->next = newNode;
        newNode->next = node;
        return retval;
    }

    /*
     * Removes a node with an key, returns false if the key is not in the set
     */
    bool remove(const K& key) {
        Node *prev, *node;
        find(key, prev, node);
        bool retval = (node != tail && key == node->key);
        if (!retval) return retval;
        prev->next = node->next;
        TM::tmDelete(node);
        return retval;
    }

    /*
     * Returns true if it finds a node with a matching key
     */
    bool contains(const K& key) {
        Node *prev, *node;
        find(key, prev, node);
        return (node != tail && key == node->key);
    }

    void find(const K& key, Node*& prev, Node*& node) {
        for (prev = head; (node = prev->next) != tail; prev = node){
            if ( !(node->key < key) ) break;
        }
    }

    // Used only for benchmarks
    bool addAll(K** keys, const int size) {
        for (int i = 0; i < size; i++) add(*keys[i]);
        return true;
    }
};

#endif /* _PERSISTENT_SEQUENTIAL_LINKED_LIST_SET_H_ */
