#ifndef _HLKVDS_KVDB_IMPL_H_
#define _HLKVDS_KVDB_IMPL_H_

#include <list>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "Db_Structure.h"
#include "hlkvds/Options.h"
#include "hlkvds/Status.h"
#include "hlkvds/Write_batch.h"
#include "hlkvds/Iterator.h"
#include "BlockDevice.h"
#include "SuperBlockManager.h"
#include "IndexManager.h"
#include "SegmentManager.h"
#include "GcManager.h"
#include "WorkQueue.h"
#include "Segment.h"
#include "ReadCache.h"

namespace hlkvds {

class KVDS {
public:
    static KVDS* Create_KVDS(const char* filename, Options opts);
    static KVDS* Open_KVDS(const char* filename, Options opts);

    Status Insert(const char* key, uint32_t key_len, const char* data,
                  uint16_t length);
    Status Get(const char* key, uint32_t key_len, string &data);
    Status Delete(const char* key, uint32_t key_len);

    Status InsertBatch(WriteBatch *batch);

    Iterator* NewIterator();

    void Do_GC();
    void ClearReadCache() {
        bdev_->ClearReadCache();
    }
    void printDbStates();

    uint32_t getReqQueSize() {
        return reqQue_.length();
    }
    uint32_t getSegWriteQueSize() {
        return segWriteQue_.length();
    }
    uint32_t getSegReaperQueSize() {
        return segReaperQue_.length();
    }

    virtual ~KVDS();

private:
    KVDS(const string& filename, Options opts);
    Status openDB();
    Status closeDB();
    bool writeMetaDataToDevice();
    bool readMetaDataFromDevice();
    void startThds();
    void stopThds();

    Status insertKey(KVSlice& slice);
    Status updateMeta(Request *req);

    Status readData(KVSlice& slice, string &data);

private:
    SuperBlockManager* sbMgr_;
    IndexManager* idxMgr_;
    BlockDevice* bdev_;
    SegmentManager* segMgr_;
    GcManager* gcMgr_;
    dslab::ReadCache* rdCache_;// readcache, rmd160, slru/lru
    string fileName_;

    SegForReq *seg_;
    std::mutex segMtx_;
    Options options_;

    // Request Merge thread
private:
    std::thread reqMergeT_;
    std::atomic<bool> reqMergeT_stop_;
    WorkQueue<Request*> reqQue_;
    void ReqMergeThdEntry();

    // Seg Write to device thread
private:
    std::vector<std::thread> segWriteTP_;
    std::atomic<bool> segWriteT_stop_;
    WorkQueue<SegForReq*> segWriteQue_;
    void SegWriteThdEntry();

    // Seg Timeout thread
private:
    std::thread segTimeoutT_;
    std::atomic<bool> segTimeoutT_stop_;
    void SegTimeoutThdEntry();

    // Seg Reaper thread
private:
    std::thread segReaperT_;
    std::atomic<bool> segReaperT_stop_;
    WorkQueue<SegForReq*> segReaperQue_;
    void SegReaperThdEntry();

    //GC thread
private:
    std::thread gcT_;
    std::atomic<bool> gcT_stop_;

    void GCThdEntry();
};

} // namespace hlkvds

#endif  // #ifndef _HLKVDS_KVDB_IMPL_H_
