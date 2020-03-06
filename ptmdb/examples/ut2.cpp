#include <cassert>
#include <string>
#include <iostream>

#include "db.h"


int main(void) {
    ptmdb::DB* db;
    ptmdb::Options options {};
    options.create_if_missing = true;
    ptmdb::Status status = ptmdb::DB::Open(options, "/tmp/testdb", &db);
    assert(status.ok());

    std::string key1str {"This is key one"};
    std::string key2str {"This is key two"};
    std::string key3str {"This is key three"};
    std::string value1str {"This is value one"};
    std::string value2str {"This is value two"};
    std::string value3str {"This is value three"};
    ptmdb::Slice key1 {key1str};
    ptmdb::Slice key2 {key2str};
    ptmdb::Slice key3 {key3str};
    ptmdb::Slice value1 {value1str};
    ptmdb::Slice value2 {value2str};
    ptmdb::Slice value3 {value3str};
    std::string value;
    ptmdb::Status s{};

    printf("Gets\n");
    // Check that none of the keys exist in the db
    s = db->Get(ptmdb::ReadOptions(), key1, &value);
    assert(s.IsNotFound());
    s = db->Get(ptmdb::ReadOptions(), key2, &value);
    assert(s.IsNotFound());
    s = db->Get(ptmdb::ReadOptions(), key3, &value);
    assert(s.IsNotFound());

    printf("\nPuts\n");
    // Insert the keys in the db
    s = db->Put(ptmdb::WriteOptions(), key1, value1);
    assert(s.ok());
    //return 0;
    s = db->Put(ptmdb::WriteOptions(), key2, value2);
    assert(s.ok());
    s = db->Put(ptmdb::WriteOptions(), key3, value3);
    assert(s.ok());

    printf("Gets\n");
    // Check that all keys are in the db
    s = db->Get(ptmdb::ReadOptions(), key1, &value);
    assert(s.ok());
    assert(value == value1);
    s = db->Get(ptmdb::ReadOptions(), key2, &value);
    assert(s.ok());
    assert(value == value2);
    s = db->Get(ptmdb::ReadOptions(), key3, &value);
    assert(s.ok());
    assert(value == value3);

    printf("Deletes\n");
    // Remove all keys from the db in a different order and check each is no longer present
    s = db->Delete(ptmdb::WriteOptions(), key2);
    assert(s.ok());
    s = db->Get(ptmdb::ReadOptions(), key2, &value);
    assert(s.IsNotFound());

    s = db->Delete(ptmdb::WriteOptions(), key1);
    assert(s.ok());
    s = db->Get(ptmdb::ReadOptions(), key1, &value);
    assert(s.IsNotFound());

    s = db->Delete(ptmdb::WriteOptions(), key3);
    assert(s.ok());
    s = db->Get(ptmdb::ReadOptions(), key3, &value);
    assert(s.IsNotFound());


    s = db->Delete(ptmdb::WriteOptions(), key2);
    assert(s.IsNotFound());

    s = db->Delete(ptmdb::WriteOptions(), key1);
    assert(s.IsNotFound());

    s = db->Delete(ptmdb::WriteOptions(), key3);
    assert(s.IsNotFound());

    delete db;
    std::cout<<"Test Passed\n";
}
