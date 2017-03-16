//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef DB_BENCH_MY_KV_H
#define DB_BENCH_MY_KV_H
class my_kv_interface {
public:
    my_kv_interface() {
    }
    ~my_kv_interface();
    virtual int open(const char* db_name) = 0;
    virtual int destroy(const char* db_name) = 0;
    virtual int put(const char* key, const char* val) = 0;
    virtual const char* get(const char* key) = 0;
    virtual int close() = 0;
private:
    my_kv_interface(const my_kv_interface&);
    void operator=(const my_kv_interface&);
};

#endif
