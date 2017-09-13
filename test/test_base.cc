#include "test_base.h"

using namespace hlkvds;

KVDS* TestBase::Create_DB(int db_size) {
    int ht_size = db_size;
    int segment_size = SEGMENT_SIZE;

    opts.hashtable_size = ht_size;
    opts.segment_size = segment_size;
    opts.gc_upper_level = 0.7;
    opts.disable_cache = 1;
    return KVDS::Create_KVDS(FILENAME, opts);
}
