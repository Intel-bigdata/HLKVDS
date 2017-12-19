#ifndef SLUR_MAP_H
#define SLUR_MAP_H

#include <pthread.h>
#include <iostream>
#include <vector>
#include "LRUMap.h"

namespace dslab{
template <class K, class D>
class SLRUMap: public CacheMap<K, D>{
	public: 	
		SLRUMap(size_t size1 = 1024, size_t size2 = 1024):
			protect(size1),probation(size2){}
		virtual ~SLRUMap(){}
		bool Put(const K& key, const D& data, K& ukey, D& udata, bool same = false);
		bool Get(const K& key, D& data);
		bool Delete(const K& key);
	private:
		LRUMap<K,D> protect;
		LRUMap<K,D> probation;

};

template <class K, class D>
bool SLRUMap<K,D>::Put(const K& key , const D& data, K& ukey, D& udata, bool same){
	D ldata;
	if( Get(key, ldata) ){//key exists, it should be now in protect segment, the segments should have been renewed
		return protect.Put(key, data, ukey, udata, same);//always false, no kv poped
	}else{// key does not exist
		return probation.Put(key, data, ukey, udata, same);//poped out or not is determined by probation segment
	}
}

template <class K, class D>
bool SLRUMap<K,D>::Get(const K& key, D& data){
	K ukey, lkey;
	D udata, ldata;
	if( probation.Get(key, data) ){//key exists in probation
		bool poped = protect.Put(key, data, ukey, udata);//kv poped out
		probation.Delete(key);//need a place to insert abandoned key from protected list
		if( poped ){
			probation.Put(ukey, udata, lkey, ldata);
		}
		return true;
	}else{ 
		return protect.Get(key,data);
	}
}

template <class K, class D>
bool SLRUMap<K,D>::Delete(const K& key){
	if( protect.Delete(key) || probation.Delete(key) ){//may need detach & attach nodes.
		return true;
	}
	return false;
}
}
#endif
