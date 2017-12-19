#ifndef _HLKVDS_OPTIONS_H_
#define _HLKVDS_OPTIONS_H_

#include <stdint.h>

namespace hlkvds {
struct Options {
    //use in readCache
    bool disable_cache;
    bool is_segmented;
    bool is_dedup;
    int cache_policy;
    int hash_code;
    int cache_size;
    int slru_percent;

    //use in Create DB
    int segment_size;
    int hashtable_size;
    int datastor_type;
    //int data_aligned_size;

    //use in Open DB
    int expired_time;
    int seg_write_thread;
    int shards_num;
    double seg_full_rate;
    double gc_upper_level;
    double gc_lower_level;

    Options();
};
} // namespace hlkvds

#endif //#define _HLKVDS_OPTIONS_H_
