//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include "test_base.h"

using namespace kvdb;

KvdbDS* TestBase::Create_DB(int db_size) {
    int ht_size = db_size;
    int segment_size = SEGMENT_SIZE;

    opts.hashtable_size = ht_size;
    opts.segment_size = segment_size;
    opts.gc_upper_level = 0.7;

    return KvdbDS::Create_KvdbDS(FILENAME, opts);
}
