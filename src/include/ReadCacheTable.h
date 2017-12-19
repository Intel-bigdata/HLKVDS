#ifndef READ_CACHE_TABLE_H
#define READ_CACHE_TABLE_H

#include <cstdint> 
#include <iostream> 
#include <functional>
#include <vector>
#include <mutex> 
#include "ReadCacheSegment.h"
using namespace std;

namespace dslab //Concurrent Thread Safe Library
{
    //The class represting the hash map.
    //It is expected for user defined types, the hash function will be provided.
    //By default, the std::hash function will be used
    //If the hash size is not provided, then a defult size of 1031 will be used
    //The hash table itself consists of an array of hash buckets.
    //Each hash bucket is implemented as singly linked list with the head as a dummy node created 
    //during the creation of the bucket. All the hash buckets are created during the construction of the map.
    //Locks are taken per bucket, hence multiple threads can write simultaneously in different buckets in the hash map
    class ReadCacheTable: public ReadCache
    {
        public:
            ReadCacheTable(bool is_dedup = 0, CachePolicy policy = LRU, size_t hashSize_ = HASH_SIZE_DEFAULT, size_t segment_size = 1024, int slru_percent = 50):hashSize(hashSize_)
            {
		findNextPrime(hashSize_);
		for(int i = 0; i < hashSize; i++){
			hashTable.push_back(new ReadCacheSegment(is_dedup, policy, segment_size, slru_percent) );//create the hash table as an array of hash buckets
		}
            }

            virtual ~ReadCacheTable()
            {
		for(int i = 0; i < hashSize; i++){
			delete hashTable[i] ;
		}
            }
        
            //Function to find an entry in the hash map matching the key.
            //If key is found, the corresponding value is copied into the parameter "value" and function returns true.
            //If key is not found, function returns false.
            bool Get(const string &key, string &value)  
            {
                size_t hashValue = hashFn(key) % hashSize ;
                return hashTable[hashValue]->Get(key, value);
            }

            //Function to insert into the hash map.
            //If key already exists, update the value, else insert a new node in the bucket with the <key, value> pair.
            void Put(const string &key, const string &value) 
            {
                size_t hashValue = hashFn(key) % hashSize ;
                hashTable[hashValue]->Put(key, value);
            }

            //Function to remove an entry from the bucket, if found
            void Delete(const string &key) 
            {
                size_t hashValue = hashFn(key) % hashSize ;
                hashTable[hashValue]->Delete(key);
            }   
	private:
		bool isPrime(size_t& n){
		    int i = 2;
		    while (i <= sqrt(n)){
			if(n%i == 0) return false;
			i++;
		    }
		    return true;
		}
		void findNextPrime(size_t& n){
		    while(!isPrime(n)) n++;
		}

        private:
            vector<ReadCacheSegment*> hashTable;
            hash<string> hashFn;
            size_t hashSize;
    };
}
#endif

