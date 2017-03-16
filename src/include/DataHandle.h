//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef _KV_DB_DATAHANDLE_H_
#define _KV_DB_DATAHANDLE_H_

#include <string>
#include <sys/types.h>

#include <list>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "Db_Structure.h"
#include "BlockDevice.h"
#include "KeyDigestHandle.h"
#include "IndexManager.h"
#include "SuperBlockManager.h"
#include "SegmentManager.h"

using namespace std;

namespace kvdb {

class SegmentOnDisk;
class DataHeader;
class DataHeaderOffset;
class HashEntry;
class IndexManager;
class SegmentManager;
class SegmentSlice;

class KVSlice {
public:
    KVSlice();
    ~KVSlice();
    KVSlice(const KVSlice& toBeCopied);
    KVSlice& operator=(const KVSlice& toBeCopied);

    KVSlice(const char* key, int key_len, const char* data, int data_len);
    KVSlice(Kvdb_Digest *digest, const char* data, int data_len);

    const Kvdb_Digest& GetDigest() const {
        return *digest_;
    }
    const char* GetKey() const {
        return key_;
    }
    const char* GetData() const {
        return data_;
    }
    string GetKeyStr() const;
    string GetDataStr() const;
    uint32_t GetKeyLen() const {
        return keyLength_;
    }
    uint16_t GetDataLen() const {
        return dataLength_;
    }
    bool IsAlignedData() const {
        return GetDataLen() == ALIGNED_SIZE;
    }
    HashEntry& GetHashEntry() const {
        return *entry_;
    }
    uint32_t GetSegId() const {
        return segId_;
    }

    void SetKeyValue(const char* key, int key_len, const char* data,
                     int data_len);
    void SetHashEntry(const HashEntry *hash_entry);
    void SetSegId(uint32_t seg_id);

private:
    const char* key_;
    uint32_t keyLength_;
    const char* data_;
    uint16_t dataLength_;
    Kvdb_Digest *digest_;
    HashEntry *entry_;
    uint32_t segId_;

    void copy_helper(const KVSlice& toBeCopied);
    void computeDigest();

};

class Request {
public:
    struct ReqStat {
        bool writed;
        bool write_stat;
        ReqStat() :
            writed(false), write_stat(false) {
        }
    };

public:
    Request();
    ~Request();
    Request(const Request& toBeCopied);
    Request& operator=(const Request& toBeCopied);
    Request(KVSlice& slice);

    KVSlice& GetSlice() const {
        return *slice_;
    }

    bool GetWriteStat() const {
        return stat_.write_stat;
    }
    void SetWriteStat(bool stat);

    void SetSeg(SegmentSlice *seg) {
        segPtr_ = seg;
    }
    SegmentSlice* GetSeg() {
        return segPtr_;
    }

    void Wait();
    void Signal();

private:
    ReqStat stat_;
    KVSlice *slice_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;

    SegmentSlice *segPtr_;
};

class SegmentSlice {
public:
    SegmentSlice();
    ~SegmentSlice();
    SegmentSlice(const SegmentSlice& toBeCopied);
    SegmentSlice& operator=(const SegmentSlice& toBeCopied);

    SegmentSlice(SegmentManager* sm, IndexManager* im, BlockDevice* bdev,
                 uint32_t timeout);

    bool TryPut(Request* req);
    void Put(Request* req);
    bool WriteSegToDevice(uint32_t seg_id);
    void Complete();
    void Notify(bool stat);
    bool IsExpired();
    uint32_t GetFreeSize() const {
        return tailPos_ - headPos_;
    }

    int32_t CommitedAndGetNum() {
        return --reqCommited_;
    }
    void CleanDeletedEntry();
    uint32_t GetSegId() const {
        return segId_;
    }

private:
    bool isCanFit(Request* req) const;
    void copyHelper(const SegmentSlice& toBeCopied);
    void fillSlice();
    void fillSegHead();
    void notifyAndClean(bool req_state);
    bool _writeDataToDevice();
    void copyToData(char* data_buff);

    uint32_t segId_;
    SegmentManager* segMgr_;
    IndexManager* idxMgr_;
    BlockDevice* bdev_;
    uint32_t timeout_;
    uint32_t segSize_;
    KVTime persistTime_;
    KVTime startTime_;

    uint32_t headPos_;
    uint32_t tailPos_;

    int32_t keyNum_;
    int32_t keyAlignedNum_;
    bool isCompleted_;
    bool hasReq_;

    std::atomic<int32_t> reqCommited_;

    std::list<Request *> reqList_;
    SegmentOnDisk *segOndisk_;

    std::mutex mtx_;
    std::list<HashEntry> delReqList_;

    char *dataBuf_;
};

} //end namespace kvdb

#endif //#ifndef _KV_DB_DATAHANDLE_H_
