//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "SuperBlockManager.h"

namespace kvdb {
bool SuperBlockManager::InitSuperBlockForCreateDB(uint64_t offset) {
    sb_ = new DBSuperBlock;
    startOff_ = offset;

    memset(sb_, 0, SuperBlockManager::SizeOfDBSuperBlock());
    return true;
}

bool SuperBlockManager::LoadSuperBlockFromDevice(uint64_t offset) {
    sb_ = new DBSuperBlock;
    startOff_ = offset;

    uint64_t length = SuperBlockManager::SizeOfDBSuperBlock();
    if ((uint64_t) bdev_->pRead(sb_, length, offset) != length) {
        __ERROR("Could not read superblock from device\n");
        return false;
    }

    return true;
}

bool SuperBlockManager::WriteSuperBlockToDevice() {
    uint64_t length = SuperBlockManager::SizeOfDBSuperBlock();
    if ((uint64_t) bdev_->pWrite(sb_, length, startOff_) != length) {
        __ERROR("Could not write superblock at position %ld\n", startOff_);
        return false;
    }
    return true;
}

void SuperBlockManager::SetSuperBlock(DBSuperBlock& sb) {
    std::lock_guard < std::mutex > l(mtx_);
    sb_->hashtable_size = sb.hashtable_size;
    sb_->number_elements = sb.number_elements;
    sb_->segment_size = sb.segment_size;
    sb_->number_segments = sb.number_segments;
    sb_->current_segment = sb.current_segment;
    sb_->db_sb_size = sb.db_sb_size;
    sb_->db_index_size = sb.db_index_size;
    sb_->db_seg_table_size = sb.db_seg_table_size;
    sb_->db_data_region_size = sb.db_data_region_size;
    sb_->device_capacity = sb.device_capacity;
    sb_->data_theory_size = sb.data_theory_size;
}

uint64_t SuperBlockManager::GetSuperBlockSizeOnDevice() {
    uint64_t sb_size_pages = SuperBlockManager::SizeOfDBSuperBlock()
            / getpagesize();
    return (sb_size_pages + 1) * getpagesize();
}

SuperBlockManager::SuperBlockManager(BlockDevice* bdev, Options &opt) :
    bdev_(bdev), sb_(NULL), options_(opt), startOff_(0) {
}

SuperBlockManager::~SuperBlockManager() {
    delete sb_;
}

void SuperBlockManager::SetElementNum(uint32_t num) {
    std::lock_guard < std::mutex > l(mtx_);
    sb_->number_elements = num;
}

void SuperBlockManager::SetCurSegId(uint32_t id) {
    std::lock_guard < std::mutex > l(mtx_);
    sb_->current_segment = id;
}

void SuperBlockManager::SetDataTheorySize(uint64_t size) {
    std::lock_guard < std::mutex > l(mtx_);
    sb_->data_theory_size = size;
}
} // namespace kvdb
