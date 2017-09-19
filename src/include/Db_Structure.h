#ifndef _HLKVDS_DB_STRUCTURE_H_
#define _HLKVDS_DB_STRUCTURE_H_

#include <stdint.h>
namespace dslab{

enum CachePolicy {LRU, SLRU};

}
namespace hlkvds {
#define MAGIC_NUMBER 0xffff0001

#define WITH_ITERATOR 1

#define RMDsize 160
#define KEYDIGEST_INT_NUM RMDsize/(sizeof(uint32_t)*8) // RIPEMD-160/(sizeof(uint32_t)*8) 160/32
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

#define FOK 0
#define ERR (-1)

typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

enum struct OpType {
    UNKOWN,
    INSERT,
    UPDATE,
    DELETE
};
} // namespace hlkvds

#endif  // #define _HLKVDS_DB_STRUCTURE_H_
