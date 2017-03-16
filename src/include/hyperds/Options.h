//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef _KV_DB_OPTIONS_H_
#define _KV_DB_OPTIONS_H_

#include <stdint.h>

namespace kvdb {
struct Options {
    //use in Create DB
    int segment_size;
    int hashtable_size;
    //int data_aligned_size;

    //use in Open DB
    int expired_time;
    int seg_write_thread;
    double seg_full_rate;
    double gc_upper_level;
    double gc_lower_level;

    Options();
};
} // namespace kvdb

#endif //#define _KV_DB_OPTIONS_H_
