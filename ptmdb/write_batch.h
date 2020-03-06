// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// WriteBatch holds a collection of updates to apply atomically to a DB.
//
// The updates are applied in the order in which they are added
// to the WriteBatch.  For example, the value of "key" will be "v3"
// after the following batch is written:
//
//    batch.Put("key", "v1");
//    batch.Delete("key");
//    batch.Put("key", "v2");
//    batch.Put("key", "v3");
//
// Multiple threads can invoke const methods on a WriteBatch without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same WriteBatch must use
// external synchronization.

#ifndef _ROMULUSDB_INCLUDE_WRITE_BATCH_H_
#define _ROMULUSDB_INCLUDE_WRITE_BATCH_H_

#include <string>
#include <vector>
#include <cstring>
#include <utility>

namespace ptmdb {

class Slice;

class WriteBatch {
 public:
  class Op{
   public:
    bool operation;
    char* key;
    int key_size;
    char* value;
    int value_size;
   Op(bool operation, char* key, int key_size, char* value, int value_size):
       operation{operation},key{key},key_size{key_size},value{value},value_size{value_size}
       {};

   // Copy constructor
   Op(const Op& other){
	   operation = other.operation;
	   key_size = other.key_size;
	   value_size = other.value_size;
	   key = new char[key_size+1];
	   value = new char[value_size+1];
	   std::memcpy(key, other.key, key_size);
	   std::memcpy(value, other.value, value_size);
	   key[key_size] = 0;
	   value[value_size] = 0;
	   //printf("copy constructor\n");
   };

   // Move constructor
   Op(Op&& o) noexcept :
           key(std::exchange(o.key, nullptr)),
           value(std::exchange(o.value, nullptr)),
           key_size(std::exchange(o.key_size, 0)),
           value_size(std::exchange(o.value_size, 0)),
           operation(std::exchange(o.operation, false)) { }

   ~Op(){
	   delete[] key;
	   delete[] value;
   }
  };

  WriteBatch();
  ~WriteBatch();

  std::vector<Op>& getTransaction();

  // Store the mapping "key->value" in the database.
  void Put(const Slice& key, const Slice& value);

  // If the database contains a mapping for "key", erase it.  Else do nothing.
  void Delete(const Slice& key);

  // Clear all updates buffered in this batch.
  void Clear();

  // Support for iterating over the contents of a batch. TODO: Is this needed?
  class Handler {
   public:
    virtual ~Handler();
    virtual void Put(const Slice& key, const Slice& value) = 0;
    virtual void Delete(const Slice& key) = 0;
  };

 private:
  std::vector<Op> trans{};

  // Intentionally copyable
};

}  // namespace ptmdb

#endif  // _ROMULUSDB_INCLUDE_WRITE_BATCH_H_
