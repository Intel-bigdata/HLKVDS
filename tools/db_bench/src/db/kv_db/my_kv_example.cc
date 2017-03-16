//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <iostream>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "db_bench.hpp"  
#include "my_kv.h"
#include "Kvdb_Impl.h"

const int DB_SIZE = 2000;
using namespace kvdb;

my_kv_interface::~my_kv_interface() {
}
class kv_db_interface : public my_kv_interface {
public:
    kv_db_interface() :
        db(NULL) {
    }
    ~kv_db_interface() {
    }

    virtual int open(const char* db_name);
    virtual int destroy(const char* db_name);
    virtual int put(const char* key, const char* val);
    virtual const char* get(const char* key);
    virtual int close();
private:
    KvdbDS *db;
    int db_size;
    int Create_DB(string filename, int db_size);
    std::string temp;
};

double timeval_diff(const struct timeval * const start,
                    const struct timeval * const end) {
    /* Calculate the second difference*/
    double r = end->tv_sec - start->tv_sec;

    /* Calculate the microsecond difference */
    if (end->tv_usec > start->tv_usec)
        r += (end->tv_usec - start->tv_usec) / 1000000.0;
    else if (end->tv_usec < start->tv_usec)
        r -= (start->tv_usec - end->tv_usec) / 1000000.0;

    return r;
}

int kv_db_interface::Create_DB(string filename, int db_size) {
    //int ht_size = db_size * 2;
    int ht_size = db_size;
    double delete_ratio = 0.9;
    double load_ratio = 0.8;
    int segment_size = 256 * 1024;

    struct timeval tv_start, tv_end;
    gettimeofday(&tv_start, NULL);

    KvdbDS *db = KvdbDS::Create_KvdbDS(filename.c_str(), ht_size, delete_ratio,
                                       load_ratio, segment_size);

    if (db->WriteMetaDataToDevice() < 0) {
        std::cout << "Create DB failed !" << std::endl;
        return -1;
    }
    gettimeofday(&tv_end, NULL);
    double diff_time = timeval_diff(&tv_start, &tv_end);
    std::cout << "Create DB use time: " << diff_time << "s" << std::endl;
    delete db;
    db = NULL;
    return 0;
}

//TODO - the default size should be configurable.
//TODO - overflow problem.
//TODO - error sys.
int kv_db_interface::open(const char* db_name) {
    int ret;
    if ((ret = Create_DB(db_name, DB_SIZE)) < 0) {
goto    ERROR;
}
db = KvdbDS::Open_KvdbDS(db_name);
return 0;
ERROR:
return ret;
}

//not support for kv-db ?
int kv_db_interface::destroy(const char* db_name) {
    if (db) {
        delete db;
        db = NULL;
    }
    return 0;
}

//@key and @value must end with '\0'
//TODO - error sys.
int kv_db_interface::put(const char* key, const char* val) {
    assert(db != NULL);
    if (!db->Insert(key, strlen(key), val, strlen(val))) {
        return -1;//error reason?
    }
    return 0;
}

const char* kv_db_interface::get(const char* key) {
    assert(db != NULL);
    if (!db->Get(key, strlen(key), temp)) {
        return NULL;
    }
    return temp.c_str();
}

int kv_db_interface::close() {
    if (db) {
        delete db;
        db = NULL;
    }
    return 0;
}

kv_db_interface my_kv_db;
int main(int argc, char* argv[]) {
    general_db_bench::Benchmark bench(argc, argv, &my_kv_db);
    bench.Run();
    return 0;
}
