/*
 * Copyright 2017-2019
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#include <algorithm>
#include <set>
#include <string>
#include <stdio.h>
#include <vector>
#include "db_impl.h"
#include "db.h"
#include "options.h"


namespace ptmdb {

Status DB::Open(const Options& options, const std::string& dbname, DB** dbptr) {
    *dbptr = new DBImpl(options, dbname);
    return Status::OK();
}

Status DestroyDB(const std::string& dbname, const Options& options) {
    //RomulusLog::init(); // TODO: fix this
    //RomulusLog::reset();
    return Status::OK();
}

}  // namespace ptmdb

