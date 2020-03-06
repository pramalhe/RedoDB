#include <cassert>
#include "ptmdb.h"
#include "db.h"
#include "slice.h"
#include "options.h"

std::string kDBPath = "/tmp/rocksdbtest-1000/dbbench";

using namespace ptmdb;

int main(void) {
    DB* db;
    Options options;

    // open DB
    Status s = DB::Open(options, kDBPath, &db);
    assert(s.ok());
    printf("RedoDB opened\n");

    // Put key-value
    s = db->Put(WriteOptions(), "key1", "value");
    assert(s.ok());
    auto endTime = steady_clock::now();
    microseconds timeus = duration_cast<microseconds>(endTime-redotimedhash::gCX.gstartTime);
    std::cout<<timeus.count()<<"\n";

    printf("RedoDB put() success\n");


    // get value
    std::string value;
    s = db->Get(ReadOptions(), "key1", &value);
    assert(s.ok());
    assert(value == "value");
    printf("RedoDB get() success\n");

    delete db;

    return 0;
}
