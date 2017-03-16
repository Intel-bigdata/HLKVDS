//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef _KV_DB_SEGMENT_MANAGER_H_
#define _KV_DB_SEGMENT_MANAGER_H_

#include <stdio.h>
#include <vector>
#include <mutex>
#include <map>

#include "Db_Structure.h"
#include "BlockDevice.h"
#include "hyperds/Options.h"
#include "SuperBlockManager.h"
#include "IndexManager.h"
#include "Utils.h"

using namespace std;

namespace kvdb {

class DataHeader;
class HashEntry;
class IndexManager;

enum struct SegUseStat {
    FREE,
    USED,
    RESERVED
};

class SegmentOnDisk {
public:
    uint64_t time_stamp;
    uint32_t checksum;
    uint32_t number_keys;
public:
    SegmentOnDisk();
    ~SegmentOnDisk();
    SegmentOnDisk(const SegmentOnDisk& toBeCopied);
    SegmentOnDisk& operator=(const SegmentOnDisk& toBeCopied);

    SegmentOnDisk(uint32_t num);
    void Update();
    void SetKeyNum(uint32_t num) {
        number_keys = num;
    }
};

class SegmentStat {
public:
    SegUseStat state;
    uint32_t free_size;
    uint32_t death_size;

public:
    SegmentStat() :
        state(SegUseStat::FREE), free_size(0), death_size(0) {
    }
    ~SegmentStat() {
    }
    SegmentStat(const SegmentStat& toBeCopied) :
        state(toBeCopied.state), free_size(toBeCopied.free_size),
                death_size(toBeCopied.death_size) {
    }
    SegmentStat& operator=(const SegmentStat& toBeCopied) {
        state = toBeCopied.state;
        free_size = toBeCopied.free_size;
        death_size = toBeCopied.death_size;
        return *this;
    }
};

class SegmentManager {
public:
    static inline size_t SizeOfSegOnDisk() {
        return sizeof(SegmentOnDisk);
    }
    static uint32_t ComputeSegNum(uint64_t total_size, uint32_t seg_size);
    static uint64_t ComputeSegTableSizeOnDisk(uint32_t seg_num);

    bool InitSegmentForCreateDB(uint64_t start_offset, uint32_t segment_size,
                                uint32_t number_segments);

    bool LoadSegmentTableFromDevice(uint64_t start_offset,
                                    uint32_t segment_size, uint32_t num_seg,
                                    uint32_t current_seg);
    bool WriteSegmentTableToDevice();

    uint32_t GetNowSegId() {
        return curSegId_;
    }
    uint32_t GetNumberOfSeg() {
        return segNum_;
    }
    uint64_t GetDataRegionSize() {
        return (uint64_t) segNum_ << segSizeBit_;
    }
    uint32_t GetSegmentSize() {
        return segSize_;
    }
    uint32_t GetMaxValueLength() {
        return maxValueLen_;
    }

    inline bool ComputeSegOffsetFromId(uint32_t seg_id, uint64_t& offset) {
        if (seg_id >= segNum_) {
            return false;
        }
        offset = dataStartOff_ + ((uint64_t) seg_id << segSizeBit_);
        return true;
    }

    inline bool ComputeSegIdFromOffset(uint64_t offset, uint32_t& seg_id) {
        if (offset < dataStartOff_ || offset > dataEndOff_) {
            return false;
        }
        seg_id = (offset - dataStartOff_) >> segSizeBit_;
        return true;
    }

    bool ComputeSegOffsetFromOffset(uint64_t offset, uint64_t& seg_offset);
    bool ComputeDataOffsetPhyFromEntry(HashEntry* entry, uint64_t& data_offset);

    bool Alloc(uint32_t& seg_id);
    bool AllocForGC(uint32_t& seg_id);
    void FreeForFailed(uint32_t seg_id);
    void FreeForGC(uint32_t seg_id);
    void Use(uint32_t seg_id, uint32_t free_size);
    void ModifyDeathEntry(HashEntry &entry);

    void SortSegsByUtils(std::multimap<uint32_t, uint32_t> &cand_map,
                         double utils);

    uint32_t GetTotalFreeSegs();
    uint32_t GetTotalUsedSegs();

    SegmentManager(BlockDevice* bdev, SuperBlockManager* sbMgr_, Options &opt);
    ~SegmentManager();

private:
    vector<SegmentStat> segTable_;
    uint64_t startOff_;
    uint64_t dataStartOff_;
    uint64_t dataEndOff_;
    uint32_t segSize_;
    uint32_t segSizeBit_;
    uint32_t segNum_;
    uint32_t curSegId_;
    uint32_t usedCounter_;
    uint32_t freedCounter_;
    uint32_t reservedCounter_;
    uint32_t maxValueLen_;

    BlockDevice* bdev_;
    SuperBlockManager* sbMgr_;
    Options &options_;
    mutable std::mutex mtx_;

};

} //end namespace kvdb
#endif // #ifndef _KV_DB_SEGMENT_MANAGER_H_
