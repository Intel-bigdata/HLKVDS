//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <leveldb/db.h>
#include <assert.h>

#include "db_bench.hpp"                                                                                       
#include "my_kv.h"

my_kv_interface::~my_kv_interface() {
}

class leveldb_interface : public my_kv_interface {
public:
    leveldb_interface() :
        db(NULL) {
    }
    ~leveldb_interface() {
    }
    virtual int open(const char* db_name);
    virtual int destroy(const char* db_name);
    virtual int put(const char* key, const char* val);
    virtual const char* get(const char* key);
    virtual int close();
private:
    leveldb::DB* db;
    std::string temp;
    leveldb::WriteOptions w_ops;
};

//no error_sys: ok - 0, error - -1 
//TODO - error_sys

int leveldb_interface::open(const char* db_name) {
    printf("[DBG] open db: %s\n", db_name);
    leveldb::Options settings;

    settings.create_if_missing = true;//TODO - 
    /*
     settings.block_cache = cache_;
     settings.write_buffer_size = FLAGS_write_buffer_size;
     settings.max_open_files = FLAGS_open_files;
     settings.filter_policy = filter_policy_;
     settings.reuse_logs = FLAGS_reuse_logs;
     */
    leveldb::Status status = leveldb::DB::Open(settings, db_name, &db);//default option
    if (!status.ok()) {
        fprintf(stderr, "open error: %s\n", status.ToString().c_str());
        return -1;
    }
    return 0;
}

int leveldb_interface::destroy(const char* db_name) {
    if (db) {
        delete db;//release the filelock
        db = NULL;
    }
    leveldb::Status status = leveldb::DestroyDB(db_name, leveldb::Options());
    return status.ok() ? 0 : -1;
}

//@key and @value end with '\0'
int leveldb_interface::put(const char* key, const char* val) {
    //	assert(db != NULL);
    //	printf("[DBG] %s, %s\n", key, val);
    leveldb::Status status = db->Put(w_ops, key, val);
    return status.ok() ? 0 : -1;
}

const char* leveldb_interface::get(const char* key) {
    assert(db != NULL);
    leveldb::Status status = db->Get(leveldb::ReadOptions(), key, &temp);
    return status.ok() ? temp.c_str() : NULL;
}

int leveldb_interface::close() {
    if (db) {
        delete db;//release the filelock
        db = NULL;
    }
    return 0;
}

class leveldb_interface my_level_db;

int main(int argc, char* argv[]) {
    general_db_bench::Benchmark bench(argc, argv, &my_level_db);
    bench.Run();
    return 0;
}
