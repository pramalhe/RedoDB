/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#ifndef _PTMDB_DB_DB_IMPL_H_
#define _PTMDB_DB_DB_IMPL_H_

#include <set>
#include "db.h"
#include "write_batch.h"
#include "ptmdb.h"
#include "TMHashMap.hpp"
#include "TMHashMapIterator.h"

namespace ptmdb {


class DBImpl : public DB {
public:
    DBImpl(const Options& raw_options, const std::string& dbname) {
        dbname_ = dbname;
        // Create the ds
#ifdef PTMDB_CAPTURE_BY_COPY
        PTM_UPDATE_TX<bool>([=] () {
            // We expect the root pointer zero to be the hashmap
            hashMap = PTM_GET_ROOT<TMHashMap>(0);
            if (hashMap == nullptr) {
                hashMap = PTM_NEW<TMHashMap>();
                PTM_PUT_ROOT(0, hashMap);
            }
            return true;
        });
#else
        PTM_UPDATE_TX([&] () {
            // We expect the root pointer zero to be the hashmap
            hashMap = PTM_GET_ROOT<TMHashMap>(0);
            if (hashMap == nullptr) {
                hashMap = PTM_NEW<TMHashMap>();
                PTM_PUT_ROOT(0, hashMap);
            }
        });
#endif
    }

    ~DBImpl() {
        // delete the hashmap
#ifdef PTMDB_CAPTURE_BY_COPY
        PTM_UPDATE_TX<bool>([=] () {
           PTM_DELETE(hashMap);
           return true;
        });
#else
        PTM_UPDATE_TX([&] () {
           PTM_DELETE(hashMap);
        });
#endif
    }

    // Set the database entry for "key" to "value".  Returns OK on success,
    // and a non-OK status on error.
    // Note: consider setting options.sync = true.
    Status Put(const WriteOptions&, const Slice& k, const Slice& v) {
#ifdef PTMDB_CAPTURE_BY_COPY
        PTM_UPDATE_TX<bool>([this,key = k, val = v] () {
            hashMap->innerPut(key,val);
           return true;
        });
#else
        PTM_UPDATE_TX([&] () {
            hashMap->innerPut(k,v);
        });
#endif
        return Status::OK();
    }

    // Remove the database entry (if any) for "key".  Returns OK on
    // success, and a non-OK status on error.  It is not an error if "key"
    // did not exist in the database.
    // Note: consider setting options.sync = true.
    Status Delete(const WriteOptions& options, const Slice& k) {
#ifdef PTMDB_CAPTURE_BY_COPY
        bool found = PTM_UPDATE_TX<bool>([this,key = k] () {
            return hashMap->innerRemove(key);
        });
#else
        bool found;
        PTM_UPDATE_TX([&] () {
            found = hashMap->innerRemove(k);
        });
#endif
        if (!found) return Status::NotFound("key Not Found");
        return Status::OK();
    }

    // Apply the specified updates to the database.
    // Returns OK on success, non-OK on failure.
    Status Write(const WriteOptions& options, WriteBatch* my_batch) {
        std::vector<WriteBatch::Op>& vec = my_batch->getTransaction();
#ifdef PTMDB_CAPTURE_BY_COPY
            while (!vec.empty()) {
                PTM_UPDATE_TX<bool>([this,oper = vec[vec.size()-1]] () {
					if(oper.operation){
						hashMap->innerPut(oper.key, oper.value);
					}else{
						hashMap->innerRemove(oper.key);
					}
					return true;
                });
                vec.pop_back();
            }
#else
        PTM_UPDATE_TX([&] () {
            for(int i=0;i<vec.size();i++){
                WriteBatch::Op& operat = vec[i];
                if(operat.operation){
                    hashMap->innerPut(operat.key, operat.value);
                }else{
                    hashMap->innerRemove(operat.key);
                }
            }
        });
#endif
        return Status::OK();
    }

    long size() {
       return hashMap->sizeHM;
    }

    // If the database contains an entry for "key" store the
    // corresponding value in *value and return OK.
    //
    // If there is no entry for "key" leave *value unchanged and return
    // a status for which Status::IsNotFound() returns true.
    //
    // May return some other Status on an error.
    Status Get(const ReadOptions& options, const Slice& k, std::string* svalue) {
#ifdef PTMDB_CAPTURE_BY_COPY
        // ERROR   ERROR   ERROR: We can not have multiple threads changing the string svalue, it creates races and possibly memory reclamation issues
        bool found = PTM_READ_TX<bool>([this,key = k,svalue] () {
            return hashMap->innerGet(key,svalue);
        });
#else
        bool found;
        PTM_READ_TX([&] () {
            found = hashMap->innerGet(k,svalue);
        });
#endif
        if (!found) return Status::NotFound("key Not Found");
        return Status::OK();
    }

    // Return a heap-allocated iterator over the contents of the database.
    // The result of NewIterator() is initially invalid (caller must
    // call one of the Seek methods on the iterator before using it).
    //
    // Caller should delete the iterator when it is no longer needed.
    // The returned iterator should be deleted before this db is deleted.
    Iterator* NewIterator(const ReadOptions& options) {
        return new TMHashMapIterator(hashMap);
    }


    //virtual bool GetProperty(const Slice& property, std::string* value);
    //virtual void GetApproximateSizes(const Range* range, int n, uint64_t* sizes);

    // Record a sample of bytes read at the specified internal key.
    // Samples are taken approximately once every config::kReadBytesPeriod
    // bytes.
    void RecordReadSample(Slice key);

private:
    friend class DB;

    // Constant after construction

    //Env* const env_;
    //const InternalKeyComparator internal_comparator_;
    //const InternalFilterPolicy internal_filter_policy_;
    const Options options_;  // options_.comparator == &internal_comparator_
    bool owns_info_log_;
    bool owns_cache_;
    std::string dbname_;
    TMHashMap* hashMap {nullptr};

    // No copying allowed
    DBImpl(const DBImpl&);
    void operator=(const DBImpl&);
    /*
  const Comparator* user_comparator() const {
    return internal_comparator_.user_comparator();
  }*/
};


}  // namespace ptmdb

#endif  // _ROMULUSDB_DB_DB_IMPL_H_
