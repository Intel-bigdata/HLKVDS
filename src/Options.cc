//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include "hyperds/Options.h"
#include "Db_Structure.h"

namespace kvdb {

Options::Options() :
    segment_size(SEGMENT_SIZE),
            hashtable_size(0),
            //data_aligned_size(ALIGNED_SIZE),
            expired_time(EXPIRED_TIME), seg_write_thread(SEG_WRITE_THREAD),
            seg_full_rate(SEG_FULL_RATE), gc_upper_level(GC_UPPER_LEVEL),
            gc_lower_level(GC_LOWER_LEVEL) {
}

} //namespace kvdb
