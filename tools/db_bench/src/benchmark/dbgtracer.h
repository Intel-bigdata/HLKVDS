//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef DB_BENCH_DBGTRACER
#define DB_BENCH_DBGTRACER
#include "mutexlock.h"
#include <cstdio>

class Tracer {//a tracer for debugging
public:
    static Tracer* getInstance();
    static void destoryInstance();
    int Put(const char* str);
private:
    Tracer() :
        file_() {
    } //
    FILE* file_;

    //no copy allowed
    Tracer(const Tracer &) {
    }
    void operator=(const Tracer &) {
    }

    port::Mutex mu_;

};

#endif
