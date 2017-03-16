//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef _KV_DB_SUPERBLOCK_H_
#define _KV_DB_SUPERBLOCK_H_

#include <mutex>

#include "Db_Structure.h"
#include "BlockDevice.h"
#include "hyperds/Options.h"
#include "Utils.h"

using namespace std;

namespace kvdb {
class SuperBlockManager;

class DBSuperBlock {
public:
    //private:
    friend class SuperBlockManager;
    uint32_t magic_number;
    uint32_t hashtable_size;
    uint32_t number_elements;
    uint32_t segment_size;
    uint32_t number_segments;
    uint32_t current_segment;
    uint64_t db_sb_size;
    uint64_t db_index_size;
    uint64_t db_seg_table_size;
    uint64_t db_data_region_size;
    uint64_t device_capacity;
    uint64_t data_theory_size;

public:
    DBSuperBlock(uint32_t magic, uint32_t ht_size, uint32_t num_eles,
                 uint32_t seg_size, uint32_t num_seg, uint32_t cur_seg,
                 uint64_t sb_size, uint64_t index_size,
                 uint64_t seg_table_size, uint64_t data_region_size,
                 uint64_t dev_size, uint64_t data_size) :
        magic_number(magic), hashtable_size(ht_size),
                number_elements(num_eles), segment_size(seg_size),
                number_segments(num_seg), current_segment(cur_seg),
                db_sb_size(sb_size), db_index_size(index_size),
                db_seg_table_size(seg_table_size),
                db_data_region_size(data_region_size),
                device_capacity(dev_size), data_theory_size(data_size) {
    }

    DBSuperBlock() :
        magic_number(0), hashtable_size(0), number_elements(0),
                segment_size(0), number_segments(0), current_segment(0),
                db_sb_size(0), db_index_size(0), db_seg_table_size(0),
                db_data_region_size(0), device_capacity(0), data_theory_size(0) {
    }

    uint32_t GetMagic() const {
        return magic_number;
    }
    uint32_t GetHTSize() const {
        return hashtable_size;
    }
    uint32_t GetElementNum() const {
        return number_elements;
    }
    uint32_t GetSegmentSize() const {
        return segment_size;
    }
    uint32_t GetSegmentNum() const {
        return number_segments;
    }
    uint32_t GetCurSegmentId() const {
        return current_segment;
    }
    uint64_t GetSbSize() const {
        return db_sb_size;
    }
    uint64_t GetIndexSize() const {
        return db_index_size;
    }
    uint64_t GetSegTableSize() const {
        return db_seg_table_size;
    }
    uint64_t GetDataRegionSize() const {
        return db_data_region_size;
    }
    uint64_t GetDeviceCapacity() const {
        return device_capacity;
    }
    uint64_t GetDataTheorySize() const {
        return data_theory_size;
    }

    ~DBSuperBlock() {
    }
}__attribute__((__packed__));

class SuperBlockManager {
public:
    static inline size_t SizeOfDBSuperBlock() {
        return sizeof(DBSuperBlock);
    }
    static uint64_t GetSuperBlockSizeOnDevice();

    bool InitSuperBlockForCreateDB(uint64_t offset);

    bool LoadSuperBlockFromDevice(uint64_t offset);
    bool WriteSuperBlockToDevice();

    void SetSuperBlock(DBSuperBlock& sb);
    const DBSuperBlock& GetSuperBlock() const {
        return *sb_;
    }

    bool IsElementFull() const {
        return sb_->number_elements == sb_->hashtable_size;
    }
    uint32_t GetMagic() const {
        return sb_->magic_number;
    }
    uint32_t GetHTSize() const {
        return sb_->hashtable_size;
    }
    uint32_t GetElementNum() const {
        return sb_->number_elements;
    }
    uint32_t GetSegmentSize() const {
        return sb_->segment_size;
    }
    uint32_t GetSegmentNum() const {
        return sb_->number_segments;
    }
    uint32_t GetCurSegmentId() const {
        return sb_->current_segment;
    }
    uint64_t GetSbSize() const {
        return sb_->db_sb_size;
    }
    uint64_t GetIndexSize() const {
        return sb_->db_index_size;
    }
    uint64_t GetSegTableSize() const {
        return sb_->db_seg_table_size;
    }
    uint64_t GetDataRegionSize() const {
        return sb_->db_data_region_size;
    }
    uint64_t GetDeviceCapacity() const {
        return sb_->device_capacity;
    }
    uint64_t GetDataTheorySize() const {
        return sb_->data_theory_size;
    }

    void SetElementNum(uint32_t num);
    void SetCurSegId(uint32_t id);
    void SetDataTheorySize(uint64_t size);

    SuperBlockManager(BlockDevice* bdev, Options &opt);
    ~SuperBlockManager();

private:
    BlockDevice* bdev_;
    DBSuperBlock* sb_;

    Options &options_;

    uint64_t startOff_;

    mutable std::mutex mtx_;

};
}// namespace kvdb

#endif //#ifndef _KV_DB_SUPERBLOCK_H_
