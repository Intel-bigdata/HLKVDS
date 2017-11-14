#ifndef _HLKVDS_KVDB_IMPL_H_
#define _HLKVDS_KVDB_IMPL_H_

#include "hlkvds/Options.h"
#include "hlkvds/Status.h"
#include "hlkvds/Write_batch.h"
#include "hlkvds/Iterator.h"
#include <map>
#include <string>
#include "ReadCache.h"

namespace hlkvds {

class BlockDevice;
class SuperBlockManager;
class IndexManager;
class MetaStor;
class SimpleDS_Impl;

class KVDS {
public:
    static KVDS* Create_KVDS(const char* filename, Options opts);
    static KVDS* Open_KVDS(const char* filename, Options opts);

    Status Insert(const char* key, uint32_t key_len, const char* data,
                  uint16_t length);
    Status Get(const char* key, uint32_t key_len, std::string &data);
    Status Delete(const char* key, uint32_t key_len);

    Status InsertBatch(WriteBatch *batch);

    Iterator* NewIterator();

    void Do_GC();
    void ClearReadCache();
    void printDbStates();

    virtual ~KVDS();

private:
    KVDS(const char* filename, Options opts);
    Status openDB();
    Status closeDB();
    void startThds();
    void stopThds();

private:
    SuperBlockManager* sbMgr_;
    IndexManager* idxMgr_;
    BlockDevice* bdev_;

    std::map<std::string,std::string> rdMap;
    dslab::ReadCache* rdCache_;// readcache, rmd160, slru/lru

    MetaStor *metaStor_;
    SimpleDS_Impl *dataStor_;

    Options options_;


};

} // namespace hlkvds

#endif  // #ifndef _HLKVDS_KVDB_IMPL_H_
