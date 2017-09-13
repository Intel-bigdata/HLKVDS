#include "hlkvds/Options.h"
#include "Db_Structure.h"

namespace hlkvds {

Options::Options() :
	    segment_size(SEGMENT_SIZE),
            hashtable_size(0),
            //data_aligned_size(ALIGNED_SIZE),
            expired_time(EXPIRED_TIME), seg_write_thread(SEG_WRITE_THREAD),
            seg_full_rate(SEG_FULL_RATE), gc_upper_level(GC_UPPER_LEVEL),
            gc_lower_level(GC_LOWER_LEVEL) {
}

} //namespace hlkvds
