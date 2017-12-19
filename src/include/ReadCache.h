#ifndef READ_CACHE_H
#define READ_CACHE_H

#include "CacheMap.h"

using namespace std;
#define HASH_SIZE_DEFAULT 1031 // A prime number as hash size gives a better distribution of values in buckets
namespace dslab
{
class ReadCache{
    public:
        ReadCache(){}
        virtual ~ReadCache(){}
        virtual bool Get(const string &key, string &value) = 0;
        virtual void Put(const string &key, const string &value) = 0;
        virtual void Delete(const string &key) = 0;
        //static ReadCache *create(bool is_segmented = 0, bool is_dedup = 0, CachePolicy policy = LRU, size_t hash_code = HASH_SIZE_DEFAULT, size_t segment_size = 1024, int slru_percent = 50);
};
}//dslab

#include "ReadCacheSegment.h"
#include "ReadCacheTable.h"
/*namespace dslab {
ReadCache *create(bool is_segmented, bool is_dedup, CachePolicy policy, size_t hash_code, size_t segment_size, int slru_percent){
    if(is_segmented){
	return new ReadCacheTable( is_dedup, policy, hash_code, segment_size, slru_percent);
    }else{
        return new ReadCacheSegment(is_dedup, policy, segment_size, slru_percent);
    }
}
}//dslab*/
#endif
