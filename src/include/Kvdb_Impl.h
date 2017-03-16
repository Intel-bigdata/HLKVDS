//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef _KV_DB_KVDB_IMPL_H_
#define _KV_DB_KVDB_IMPL_H_

#include <list>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "Db_Structure.h"
#include "hyperds/Options.h"
#include "hyperds/status.h"
#include "BlockDevice.h"
#include "SuperBlockManager.h"
#include "IndexManager.h"
#include "DataHandle.h"
#include "SegmentManager.h"
#include "GcManager.h"
#include "WorkQueue.h"

namespace kvdb {

class KvdbDS {
public:
    static KvdbDS* Create_KvdbDS(const char* filename, Options opts);
    static KvdbDS* Open_KvdbDS(const char* filename, Options opts);

    Status Insert(const char* key, uint32_t key_len, const char* data,
                  uint16_t length);
    Status Get(const char* key, uint32_t key_len, string &data);
    Status Delete(const char* key, uint32_t key_len);

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

    virtual ~KvdbDS();

private:
    KvdbDS(const string& filename, Options opts);
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
    string fileName_;

    SegmentSlice *seg_;
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
    WorkQueue<SegmentSlice*> segWriteQue_;
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
    WorkQueue<SegmentSlice*> segReaperQue_;
    void SegReaperThdEntry();

    //GC thread
private:
    std::thread gcT_;
    std::atomic<bool> gcT_stop_;

    void GCThdEntry();
};

} // namespace kvdb

#endif  // #ifndef _KV_DB_KVDB_IMPL_H_
