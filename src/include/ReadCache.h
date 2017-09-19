#ifndef READ_CACHE_H
#define READ_CACHE_H
#include <iostream>
#include <string.h>
#include <map>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "CacheMap.h"
#include "KeyDigestHandle.h"
namespace dslab{
typedef boost::shared_mutex smutex;
typedef boost::unique_lock< smutex > WriteLock;
typedef boost::shared_lock< smutex > ReadLock;

class ReadCache{
	public:
		ReadCache(CachePolicy policy, size_t cache_size = 1024, int percent = 50);
		~ReadCache();
		void Put(string key, string value);
		bool Get(string key, string& value);
		void Delete(string key);
	private:
		CacheMap<string, string>* cache_map;//map<footprint,value>
		map<string,string> dedup_map;//map<key,footprint>
		multimap<string, string> refer_map;//<footprint,keys>
		hlkvds::KeyDigestHandle *em;//input digest to footprint
		smutex myLock;
};

}

#endif
