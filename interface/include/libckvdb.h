//Copyright [2017] <Intel>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*callback_t)(void* comp, void *arg);
typedef void* kvdb_completion_t;
typedef void* kvdb_ioctx_t;

void kvdb_aio_release(kvdb_completion_t c);
void kvdb_aio_wait_for_complete(kvdb_completion_t c);
ssize_t kvdb_aio_get_return_value(kvdb_completion_t c);
int kvdb_aio_create_completion(void *cb_arg, callback_t complete_cb, kvdb_completion_t *c);

int kvdb_open(kvdb_ioctx_t *io, const char* name, bool disable_cache, bool isSegmented, bool isDedup, int cache_policy, int hash_code, int cache_size, int slru_percent);
int kvdb_close(kvdb_ioctx_t io);
int kvdb_aio_read(kvdb_ioctx_t io, char* data, uint64_t offset, uint64_t length, kvdb_completion_t c);
int kvdb_aio_write(kvdb_ioctx_t io, const char* data, uint64_t offset, uint64_t length, kvdb_completion_t c );

#ifdef __cplusplus
} //end extern "C"
#endif
