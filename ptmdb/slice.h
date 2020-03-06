// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Slice is a simple structure containing a pointer into some external
// storage and a size.  The user of a Slice must ensure that the slice
// is not used after the corresponding external storage has been
// deallocated.
//
// Multiple threads can invoke const methods on a Slice without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same Slice must use
// external synchronization.

#ifndef _RDB_INCLUDE_SLICE_H_
#define _RDB_INCLUDE_SLICE_H_

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>
#include "ptmdb.h"

namespace ptmdb {

class Slice {
 public:
  // Create an empty slice.
  Slice() : data_(""), size_(0) { }

  ~Slice() { if (isreplica) delete[] data_; }

  // Create a slice that refers to d[0,n-1].
  Slice(const char* d, size_t n) : data_(d), size_(n) { }

  // Create a slice that refers to the contents of "s"
  Slice(const std::string& s) : data_(s.data()), size_(s.size()) { }

  // Create a slice that refers to s[0,strlen(s)-1]
  Slice(const char* s) : data_(s), size_(strlen(s)) { }

  // Copy constructor
  Slice(const Slice& other){
    size_ = other.size_;
    data_ = new char[size_+1];
    std::memcpy((void*)data_, (void*)other.data_, size_+1);
    isreplica = true; // Tell the destructor to cleanup data_
    //printf("Slice copy constructor size=%ld\n", size_);
  }

  // Return a pointer to the beginning of the referenced data
  const char* data() const { return data_; }

  // Return the length (in bytes) of the referenced data
  size_t size() const { return size_; }

  // Return true iff the length of the referenced data is zero
  bool empty() const { return size_ == 0; }

  // Return the ith byte in the referenced data.
  // REQUIRES: n < size()
  char operator[](size_t n) const {
    assert(n < size());
    return data_[n];
  }

  // Change this slice to refer to an empty array
  void clear() { data_ = ""; size_ = 0; }

  // Drop the first "n" bytes from this slice.
  void remove_prefix(size_t n) {
    assert(n <= size());
    data_ += n;
    size_ -= n;
  }

  // Return a string that contains the copy of the referenced data.
  std::string ToString() const { return std::string(data_, size_); }

  // Three-way comparison.  Returns value:
  //   <  0 iff "*this" <  "b",
  //   == 0 iff "*this" == "b",
  //   >  0 iff "*this" >  "b"
  int compare(const Slice& b) const;

  // Return true iff "x" is a prefix of "*this"
  bool starts_with(const Slice& x) const {
    return ((size_ >= x.size_) &&
            (std::memcmp(data_, x.data_, x.size_) == 0));
  }

  uint64_t toHash() const {
      uint64_t h = 5381;
      for (size_t i = 0; i < size_; i++) {
          h = ((h << 5) + h) + data_[i]; /* hash * 33 + data_[i] */
      }
      return h;
  }

private:
  const char* data_ {nullptr};
  size_t size_ {0};
  bool isreplica {false};
};

inline bool operator==(const Slice& x, const Slice& y) {
  return ((x.size() == y.size()) &&
          (std::memcmp(x.data(), y.data(), x.size()) == 0));
}

inline bool operator!=(const Slice& x, const Slice& y) {
  return !(x == y);
}

inline int Slice::compare(const Slice& b) const {
  const size_t min_len = (size_ < b.size_) ? size_ : b.size_;
  int r = std::memcmp(data_, b.data_, min_len);
  if (r == 0) {
    if (size_ < b.size_) r = -1;
    else if (size_ > b.size_) r = +1;
  }
  return r;
};


// We need this for persistency because the contents have to be copied into persistent memory
class PSlice {
public:
	PSlice() { }
	
    // Creates a copy of an existing Slice
    PSlice(const Slice& sl) {
        size_ = sl.size();
        data_ = (char*)TM_PMALLOC(size_+1);
        uint8_t* _addr = (uint8_t*)data_.pload();
        size_t _size = size_.pload();
#ifdef USE_CXREDO
        uint64_t offset = cxredo::tlocal.tl_cx_size;
        PTM_LOG(_addr,(uint8_t*)sl.data(),_size);
#elif defined USE_CXREDOTIMED
        uint64_t offset = cxredotimed::tlocal.tl_cx_size;
        PTM_LOG(_addr,(uint8_t*)sl.data(),_size);
#elif defined USE_REDOTIMEDHASH
        uint64_t offset = redotimedhash::tlocal.tl_cx_size;
        PTM_LOG(_addr,(uint8_t*)sl.data(),_size);
#else
        uint64_t offset = 0;
#endif
        std::memcpy(_addr+offset, sl.data(), _size);
        *reinterpret_cast<char*>(_addr+_size+offset) = 0;
        PTM_FLUSH((uint8_t*)_addr, _size+1);
    }

    // Creates a copy of an existing PSlice (Copy Constructor). Not being used
    PSlice(const PSlice& psl) {
        size_ = psl.size();
        data_ = (char*)TM_PMALLOC(size_+1);
        uint8_t* _addr = (uint8_t*)data_.pload();
        size_t _size = size_.pload();
#ifdef USE_CXREDO
        uint64_t offset = cxredo::tlocal.tl_cx_size;
        PTM_LOG(_addr,(uint8_t*)psl.data(),_size);
#elif defined USE_CXREDOTIMED
        uint64_t offset = cxredotimed::tlocal.tl_cx_size;
        PTM_LOG(_addr,(uint8_t*)psl.data(),_size);
#elif defined USE_REDOTIMEDHASH
        uint64_t offset = redotimedhash::tlocal.tl_cx_size;
        PTM_LOG(_addr,(uint8_t*)psl.data(),_size);
#else
        uint64_t offset = 0;
#endif
        std::memcpy(_addr+offset, psl.data(), _size+1);
        PTM_FLUSH((uint8_t*)_addr, _size+1);
    }

    ~PSlice() {
        TM_PFREE(data_.pload());
    }

    // Return a pointer to the beginning of the referenced data
    const char* data() const {
#ifdef USE_CXREDO
        uint64_t offset = cxredo::tlocal.tl_cx_size;
#elif defined USE_CXREDOTIMED
        uint64_t offset = cxredotimed::tlocal.tl_cx_size;
#else
        uint64_t offset = 0;
#endif
    	return data_.pload()+offset;
    }

    // Return the length (in bytes) of the referenced data
    size_t size() const { return size_.pload(); }

    bool operator == (const Slice& other) {
        return (size() == other.size() && std::memcmp(data(), other.data(), size()) == 0);
    }

    bool operator == (const PSlice& other) {
        return (size() == other.size() && std::memcmp(data(), other.data(), size()) == 0);
    }

    // Assignment operator
    PSlice& operator=(const PSlice& psl) {
        size_ = psl.size();
        TM_PFREE(data_.pload());
        data_ = (char*)TM_PMALLOC(size_+1);
        uint8_t* _addr = (uint8_t*)data_.pload();
        size_t _size = size_.pload();
#ifdef USE_CXREDO
        uint64_t offset = cxredo::tlocal.tl_cx_size;
        PTM_LOG(_addr,(uint8_t*)psl.data(),_size);
#elif defined USE_CXREDOTIMED
        uint64_t offset = cxredotimed::tlocal.tl_cx_size;
        PTM_LOG(_addr,(uint8_t*)psl.data(),_size);
#elif defined USE_REDOTIMEDHASH
        uint64_t offset = redotimedhash::tlocal.tl_cx_size;
        PTM_LOG(_addr,(uint8_t*)psl.data(),_size);
#else
        uint64_t offset = 0;
#endif
        std::memcpy(_addr+offset, psl.data(), _size+1);
        PTM_FLUSH((uint8_t*)_addr, _size+1);
        return *this;
    }

    uint64_t toHash() const {
#ifdef USE_CXREDO
        uint64_t offset = cxredo::tlocal.tl_cx_size;
#elif defined USE_CXREDOTIMED
        uint64_t offset = cxredotimed::tlocal.tl_cx_size;
#elif defined USE_REDOTIMEDHASH
        uint64_t offset = redotimedhash::tlocal.tl_cx_size;
#else
        uint64_t offset = 0;
#endif
        uint64_t h = 5381;
        char* data = data_.pload()+offset;
        size_t size = size_.pload();
        for (size_t i = 0; i < size; i++) {
            h = ((h << 5) + h) + data[i]; /* hash * 33 + data_[i] */ // pload() on every data[i]
        }
        return h;
    }

private:
    TM_TYPE<char*> data_ {nullptr};
    TM_TYPE<size_t> size_ {0};
};


}  // namespace ptmdb

namespace std {
    template <>
    struct hash<ptmdb::Slice> {
        std::size_t operator()(const ptmdb::Slice& k) const {
            using std::size_t;
            using std::hash;
            return (hash<uint64_t>()(k.toHash()));
        }
    };

    template <>
    struct hash<ptmdb::PSlice> {
        std::size_t operator()(const ptmdb::PSlice& k) const {
            using std::size_t;
            using std::hash;
            return (hash<uint64_t>()(k.toHash()));
        }
    };
}

#endif
