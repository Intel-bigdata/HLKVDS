#ifndef _HLKVDS_DB_STRUCTURE_H_
#define _HLKVDS_DB_STRUCTURE_H_

#include <stdint.h>

namespace hlkvds {
#define MAGIC_NUMBER 0xffff0001

#define DISABLE_CACHE 1
#define IS_SEGMENTED 0
#define IS_DEDUP 0
#define CACHE_POLICY 0 // 0:LRU 1:SLRU
#define HASH_CODE 1031	
#define CACHE_SIZE 64
#define SLRU_PERCENT 50	

#define SEG_RESERVED_FOR_GC 2

//default Options
#define SEGMENT_SIZE 256 * 1024
#define EXPIRED_TIME 1000 // unit microseconds
#define ALIGNED_SIZE 4096

#define SEG_WRITE_THREAD 10
#define SEG_FULL_RATE 0.9
#define CAPACITY_THRESHOLD_TODO_GC 0.5
#define GC_UPPER_LEVEL 0.3
#define GC_LOWER_LEVEL 0.1

#define FIO
//#define DEBUG
#define INFO
#define WARN
#define ERROR

#ifdef DEBUG
#include <pthread.h>
#define __DEBUG(x...) do {                                    \
        fprintf(stderr,"[DEBUG] [Pid:%ld] %s (%s:%d) : ",(uint64_t)pthread_self(), __FUNCTION__, __FILE__,__LINE__); \
        fprintf(stderr,##x);             \
        fprintf(stderr,"\n");              \
}while(0)
#else
#define __DEBUG(x...)
#endif

#ifdef INFO
#include <pthread.h>
#define __INFO(x...) do {                                    \
        fprintf(stderr,"[INFO]  [Pid:%ld] %s (%s:%d) : ",(uint64_t)pthread_self(), __FUNCTION__, __FILE__,__LINE__); \
        fprintf(stderr,##x);             \
        fprintf(stderr,"\n");              \
}while(0)
#else
#define __INFO(x...)
#endif

#ifdef WARN
#include <pthread.h>
#define __WARN(x...) do {                                    \
        fprintf(stderr,"[WARN]  [Pid:%ld] %s (%s:%d) : ",(uint64_t)pthread_self(), __FUNCTION__, __FILE__,__LINE__); \
        fprintf(stderr,##x);             \
        fprintf(stderr,"\n");              \
}while(0)
#else
#define __WARN(x...)
#endif

#ifdef ERROR
#include <pthread.h>
#define __ERROR(x...) do {                                    \
        fprintf(stderr,"[ERROR] [Pid:%ld] %s (%s:%d) : ",(uint64_t)pthread_self(), __FUNCTION__, __FILE__,__LINE__); \
        fprintf(stderr,##x);             \
        fprintf(stderr,"\n");              \
}while(0)
#else
#define __ERROR(x...)
#endif

} // namespace hlkvds

#endif  // #define _HLKVDS_DB_STRUCTURE_H_
