// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// WriteBatch::rep_ :=
//    sequence: fixed64
//    count: fixed32
//    data: record[count]
// record :=
//    kTypeValue varstring varstring         |
//    kTypeDeletion varstring
// varstring :=
//    len: varint32
//    data: uint8[len]

#include "write_batch.h"
#include "db.h"
#include "status.h"


namespace ptmdb {


WriteBatch::WriteBatch() { }


WriteBatch::~WriteBatch() { }


WriteBatch::Handler::~Handler() { }

void WriteBatch::Clear() {
    trans.clear();
}

std::vector<WriteBatch::Op>& WriteBatch::getTransaction() {
    return trans;
}

void WriteBatch::Put(const Slice& key, const Slice& value) {
    char* k = new char[key.size()+1];
    char* v = new char[value.size()+1];
    std::memcpy(k, key.data(), key.size());
    std::memcpy(v, value.data(), value.size());
    k[key.size()] = 0;
    v[value.size()] = 0;
    Op operat {true, k, (int)key.size(), v, (int)value.size()};
    trans.push_back(std::move(operat));
}

void WriteBatch::Delete(const Slice& key) {
    char* k = new char[key.size()+1];
    std::memcpy(k, key.data(), key.size());
    k[key.size()] = 0;
    Op operat {false, k, (int)key.size(), nullptr, 0};
    trans.push_back(std::move(operat));
}


}  // namespace ptmdb
