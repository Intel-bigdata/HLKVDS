#include "hlkvds/Options.h"
#include "Db_Structure.h"

namespace hlkvds {

Options::Options() :
	disable_cache(DISABLE_CACHE),
	is_segmented(IS_SEGMENTED),
	is_dedup(IS_DEDUP),
        cache_policy(CACHE_POLICY),
	hash_code(HASH_CODE),
        cache_size(CACHE_SIZE),
	slru_percent(SLRU_PERCENT),
	    segment_size(SEGMENT_SIZE),
        hashtable_size(0),
        datastor_type(0),
        //data_aligned_size(ALIGNED_SIZE),
        expired_time(EXPIRED_TIME),
        seg_write_thread(SEG_WRITE_THREAD),
        shards_num(1),
        seg_full_rate(SEG_FULL_RATE),
        gc_upper_level(GC_UPPER_LEVEL),
        gc_lower_level(GC_LOWER_LEVEL) {
}

} //namespace hlkvds
