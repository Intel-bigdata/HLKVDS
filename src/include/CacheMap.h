#ifndef CACHE_MAP_H
#define CACHE_MAP_H
#include <pthread.h>
#include <iostream>
#include <vector>
#include <unordered_map>

namespace dslab {

enum CachePolicy { LRU, SLRU };

template <class K, class D>
class CacheMap{
public:
    CacheMap(){}
    virtual ~CacheMap(){}
    virtual bool Put(const K& key, const D& data, K& update_key, D& update_data, bool same = false) = 0;
    virtual bool Get(const K& key, D& data) = 0;
    virtual bool Delete(const K& key) = 0;
    static CacheMap *create(CachePolicy policy, size_t cache_size = 1024, size_t probation = 1024);

};
}

#include "LRUMap.h"
#include "SLRUMap.h"
namespace dslab {

template <class K, class D>
CacheMap<K, D> *CacheMap<K, D>::create(CachePolicy policy, size_t cache_size, size_t probation){
	if(policy == LRU){
		return new LRUMap<K, D> (cache_size);
	}else{
		size_t protect = cache_size-probation;
		return new SLRUMap<K, D> (protect, probation);
	} 
} 
}
#endif
