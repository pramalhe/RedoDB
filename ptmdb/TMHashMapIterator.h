// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// An iterator yields a sequence of key/value pairs from a source.
// The following class defines the interface.  Multiple implementations
// are provided by this library.  In particular, iterators are provided
// to access the contents of a Table or a DB.
//
// Multiple threads can invoke const methods on an Iterator without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same Iterator must use
// external synchronization.

#ifndef _TM_HASHMAP_ITERATOR_H_
#define _TM_HASHMAP_ITERATOR_H_

#include "slice.h"
#include "status.h"
#include "iterator.h"

namespace ptmdb {


class TMHashMapIterator : public Iterator {
 public:
  TMHashMap* db_hashmap = nullptr;
  // The state of the iterator is kept in the 'bucket' and 'node'
  long bucket = -1;
  TMHashMap::Node* node = nullptr;

  TMHashMapIterator(TMHashMap* db_hashmap) : db_hashmap{db_hashmap} {
  };

  ~TMHashMapIterator() {
  }

  // An iterator is either positioned at a key/value pair, or
  // not valid.  This method returns true iff the iterator is valid.
  bool Valid() const{
      if(bucket==-1) return false;
      return true;
  }

  // Position at the first key in the source.  The iterator is Valid()
  // after this call iff the source is not empty.
  void SeekToFirst(){
      if(db_hashmap->sizeHM.pload()==0){
          bucket = -1;
          return;
      }
      for(int i=0;i< db_hashmap->capacity;i++){
          if(db_hashmap->buckets[i]!=nullptr){
              bucket = i;
              node = db_hashmap->buckets[i];
              return;
          }
      }
  }

  // Position at the last key in the source.  The iterator is
  // Valid() after this call iff the source is not empty.
  void SeekToLast(){
      if(db_hashmap->sizeHM.pload()==0){
          bucket = -1;
          return;
      }
      for(int i=db_hashmap->capacity-1; i >= 0; i--){
        if(db_hashmap->buckets[i]!=nullptr){
            bucket = i;
            TMHashMap::Node* nodeLast = db_hashmap->buckets[i];
            while(nodeLast!=nullptr){
                if(nodeLast->next==nullptr){
                    node = nodeLast;
                    return;
                }
                nodeLast = nodeLast->next;
            }
        }
      }
  }

  // Position at the first key in the source that is at or past target.
  // The iterator is Valid() after this call iff the source contains
  // an entry that comes at or past target.
  void Seek(const Slice& target){
      if(db_hashmap->sizeHM.pload()==0){
          bucket = -1;
          return;
      }
      auto h = db_hashmap->getBucket(target);
      TMHashMap::Node* nodeSeek = db_hashmap->buckets[h];
      TMHashMap::Node* prev = nodeSeek;
      while(nodeSeek!=nullptr){
          if(nodeSeek->key == target){
              bucket = h;
              node = nodeSeek;
              return;
          }
          prev = nodeSeek;
          nodeSeek = nodeSeek->next;
      }
      if(prev == nullptr) {
          bucket = -1;
          return;
      }
      bucket = h;
      node = prev;
  }

  // Moves to the next entry in the source.  After this call, Valid() is
  // true iff the iterator was not positioned at the last entry in the source.
  // REQUIRES: Valid()
  void Next(){
      if(bucket == -1) return;
      TMHashMap::Node* next = node->next;
      if(next!=nullptr){
          node = next;
          return;
      }
      const int cap = db_hashmap->capacity;
      for(int i=bucket+1; i<cap ;i++){
          next = db_hashmap->buckets[i];
          if(next!=nullptr){
              bucket = i;
              node = next;
              return;
          }
      }
      bucket = -1;
  }

  // Moves to the previous entry in the source.  After this call, Valid() is
  // true iff the iterator was not positioned at the first entry in source.
  // REQUIRES: Valid()
  void Prev(){
      if(bucket == -1) return;
      TMHashMap::Node* nodeAt = db_hashmap->buckets[bucket];
      if(node==nodeAt){
          while(true){
              bucket--;
              if(bucket==-1) return;
              nodeAt = db_hashmap->buckets[bucket];
              if(nodeAt!=nullptr)break;
          }
          while(true){
              if(nodeAt->next==nullptr){
                  node = nodeAt;
                  return;
              }
              nodeAt = nodeAt->next;
          }
      }
      while(nodeAt!=node){
          if(nodeAt->next==node){
              node = nodeAt;
              return;
          }
          nodeAt = nodeAt->next;
      }
  }

  // Return the key for the current entry.  The underlying storage for
  // the returned slice is valid only until the next modification of
  // the iterator.
  // REQUIRES: Valid()
  Slice key() const{
      return node->key.data();
  }

  // Return the value for the current entry.  The underlying storage for
  // the returned slice is valid only until the next modification of
  // the iterator.
  // REQUIRES: Valid()
  Slice value() const{
      return node->val.data();
  }

  // If an error has occurred, return it.  Else return an ok status.
  Status status() const{
      return Status{};
  }

  // Clients are allowed to register function/arg1/arg2 triples that
  // will be invoked when this iterator is destroyed.
  //
  // Note that unlike all of the preceding methods, this method is
  // not abstract and therefore clients should not override it.
  //typedef void (*CleanupFunction)(void* arg1, void* arg2);
  void RegisterCleanup(CleanupFunction function, void* arg1, void* arg2){

  }

  // No copying allowed
  TMHashMapIterator(const TMHashMapIterator&);
  void operator=(const TMHashMapIterator&);
};

}  // namespace ptmdb

#endif  // _TM_HASHMAP_ITERATOR_H_
