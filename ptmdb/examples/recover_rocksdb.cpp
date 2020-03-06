#include <cassert>
#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

using namespace rocksdb;

std::string kDBPath = "/tmp/rocksdbtest-1000/dbbench";

// Before opening the DB we must run:
// time ~/rocksdb/ldb repair --db=/tmp/rocksdbtest-1000/dbbench
// Or on castor-1:
// time ~/rocksdb/ldb repair --db=/mnt/pmem0/rocksdb
//
// Running rocks with 1 thread --sync --fillrandom  on /mnt/pmem0/:
// Number of key/values (10/100)    Time for ldb to complete (miliseconds)
// 10k                                     42
// 100k                                   202
// 1M                                     221
// 10M                                   1163
//
// Running rocks with 16 threads --sync --fillrandom on /mnt/pmem0/:
// Number of key/values (10/100)    Time for ldb to complete (miliseconds)
// 10k                                    317
// 100k                                   574
// 1M                                     596
// 10M                                   1978



int main(void) {
    DB* db;
    Options options;

    // open DB
    Status s = DB::Open(options, kDBPath, &db);
    assert(s.ok());
    printf("RocksDB opened\n");

    // Put key-value
    s = db->Put(WriteOptions(), "key1", "value");
    assert(s.ok());
    printf("RocksDB put() success\n");

    // get value
    std::string value;
    s = db->Get(ReadOptions(), "key1", &value);
    assert(s.ok());
    assert(value == "value");
    printf("RocksDB get() success\n");

    delete db;

    return 0;
}
