#ifndef _HLKVDS_OPTIONS_H_
#define _HLKVDS_OPTIONS_H_

#include <stdint.h>

namespace hlkvds {
struct Options {
    //use in readCache
    bool disable_cache;
    int cache_size;
    int cache_policy;
    int slru_partition;
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
} // namespace hlkvds

#endif //#define _HLKVDS_OPTIONS_H_
