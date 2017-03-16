//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include "SegmentManager.h"
#include <math.h>

namespace kvdb {

SegmentOnDisk::SegmentOnDisk() :
    checksum(0), number_keys(0) {
    time_stamp = KVTime::GetNow();
}

SegmentOnDisk::~SegmentOnDisk() {
}

SegmentOnDisk::SegmentOnDisk(const SegmentOnDisk& toBeCopied) {
    time_stamp = toBeCopied.time_stamp;
    checksum = toBeCopied.checksum;
    number_keys = toBeCopied.number_keys;
}

SegmentOnDisk& SegmentOnDisk::operator=(const SegmentOnDisk& toBeCopied) {
    time_stamp = toBeCopied.time_stamp;
    checksum = toBeCopied.checksum;
    number_keys = toBeCopied.number_keys;
    return *this;
}

SegmentOnDisk::SegmentOnDisk(uint32_t num) :
    checksum(0), number_keys(num) {
    time_stamp = KVTime::GetNow();
}

void SegmentOnDisk::Update() {
    time_stamp = KVTime::GetNow();
}

uint64_t SegmentManager::ComputeSegTableSizeOnDisk(uint32_t seg_num) {
    uint64_t segtable_size = sizeof(time_t) + sizeof(SegmentStat) * seg_num;
    uint64_t segtable_size_pages = segtable_size / getpagesize();
    return (segtable_size_pages + 1) * getpagesize();
}

uint32_t SegmentManager::ComputeSegNum(uint64_t total_size, uint32_t seg_size) {
    uint32_t seg_num = total_size / seg_size;
    uint32_t seg_size_bit = log2(seg_size);
    uint64_t seg_table_size =
            SegmentManager::ComputeSegTableSizeOnDisk(seg_num);
    while (seg_table_size + ((uint64_t) seg_num << seg_size_bit) > total_size) {
        seg_num--;
        seg_table_size = SegmentManager::ComputeSegTableSizeOnDisk(seg_num);
    }
    return seg_num;
}

bool SegmentManager::InitSegmentForCreateDB(uint64_t start_offset,
                                            uint32_t segment_size,
                                            uint32_t number_segments) {
    uint32_t align_bit = log2(ALIGNED_SIZE);
    if (segment_size != (segment_size >> align_bit) << align_bit) {
        return false;
    }

    startOff_ = start_offset;
    segSize_ = segment_size;
    segSizeBit_ = log2(segSize_);
    segNum_ = number_segments;
    curSegId_ = 0;
    dataStartOff_ = start_offset
            + SegmentManager::ComputeSegTableSizeOnDisk(segNum_);
    dataEndOff_ = dataStartOff_ + ((uint64_t) segNum_ << segSizeBit_);

    maxValueLen_ = segSize_ - SegmentManager::SizeOfSegOnDisk()
            - IndexManager::SizeOfHashEntryOnDisk();

    freedCounter_ = segNum_;
    usedCounter_ = 0;
    reservedCounter_ = 0;

    //init segment table
    SegmentStat seg_stat;
    for (uint32_t seg_index = 0; seg_index < segNum_; seg_index++) {
        segTable_.push_back(seg_stat);
    }

    return true;
}

bool SegmentManager::LoadSegmentTableFromDevice(uint64_t start_offset,
                                                uint32_t segment_size,
                                                uint32_t num_seg,
                                                uint32_t current_seg) {
    startOff_ = start_offset;
    segSize_ = segment_size;
    segSizeBit_ = log2(segSize_);
    segNum_ = num_seg;
    //curSegId_ = current_seg;
    curSegId_ = sbMgr_->GetCurSegmentId();

    dataStartOff_ = start_offset
            + SegmentManager::ComputeSegTableSizeOnDisk(segNum_);
    dataEndOff_ = dataStartOff_ + ((uint64_t) segNum_ << segSizeBit_);

    maxValueLen_ = segSize_ - SegmentManager::SizeOfSegOnDisk()
            - IndexManager::SizeOfHashEntryOnDisk();

    uint64_t offset = startOff_;
    SegmentStat* segs_stat = new SegmentStat[segNum_];
    uint64_t length = sizeof(SegmentStat) * (uint64_t) segNum_;
    if (bdev_->pRead(segs_stat, length, offset) != (ssize_t) length) {
        __ERROR("can not write segment to device!");
        delete[] segs_stat;
        return false;
    }
    for (uint32_t seg_index = 0; seg_index < segNum_; seg_index++) {
        SegmentStat seg_stat;
        // If state is reserved, need reset segmentstat
        if (segs_stat[seg_index].state == SegUseStat::FREE) {
            seg_stat = segs_stat[seg_index];
            freedCounter_++;
        } else if (segs_stat[seg_index].state == SegUseStat::USED) {
            seg_stat = segs_stat[seg_index];
            usedCounter_++;
        } else if (segs_stat[seg_index].state == SegUseStat::RESERVED) {
            freedCounter_++;
        }
        segTable_.push_back(seg_stat);
    }
    delete[] segs_stat;
    return true;
}

bool SegmentManager::WriteSegmentTableToDevice() {
    SegmentStat* segs_stat = new SegmentStat[segNum_];
    uint64_t length = sizeof(SegmentStat) * (uint64_t) segNum_;
    for (uint32_t seg_index = 0; seg_index < segNum_; seg_index++) {
        // If state is reserved, need reset segmentstat
        if (segTable_[seg_index].state != SegUseStat::RESERVED) {
            segs_stat[seg_index] = segTable_[seg_index];
        } else {
            __WARN("Segment Maybe not write to device! seg_id = %d", seg_index);
        }
    }
    if (bdev_->pWrite(segs_stat, length, startOff_) != (ssize_t) length) {
        __ERROR("can not write segment to device!");
        delete[] segs_stat;
        return false;
    }
    delete[] segs_stat;

    sbMgr_->SetCurSegId(curSegId_);
    return true;
}

bool SegmentManager::ComputeSegOffsetFromOffset(uint64_t offset,
                                                uint64_t& seg_offset) {
    uint32_t seg_id = 0;
    if (!ComputeSegIdFromOffset(offset, seg_id)) {
        return false;
    }
    if (!ComputeSegOffsetFromId(seg_id, seg_offset)) {
        return false;
    }
    return true;
}

bool SegmentManager::ComputeDataOffsetPhyFromEntry(HashEntry* entry,
                                                   uint64_t& data_offset) {
    uint64_t seg_offset = 0;
    uint64_t header_offset = entry->GetHeaderOffsetPhy();
    if (!ComputeSegOffsetFromOffset(header_offset, seg_offset)) {
        return false;
    }
    data_offset = seg_offset + entry->GetDataOffsetInSeg();
    return true;
}

bool SegmentManager::Alloc(uint32_t& seg_id) {
    std::unique_lock < std::mutex > l(mtx_);
    if (freedCounter_ <= SEG_RESERVED_FOR_GC) {
        return false;
    }

    l.unlock();
    return AllocForGC(seg_id);
}

bool SegmentManager::AllocForGC(uint32_t& seg_id) {
    //for first use !!
    std::lock_guard < std::mutex > l(mtx_);
    if (segTable_[curSegId_].state == SegUseStat::FREE) {
        seg_id = curSegId_;
        segTable_[curSegId_].state = SegUseStat::RESERVED;

        reservedCounter_++;
        freedCounter_--;
        return true;
    }

    uint32_t seg_index = curSegId_ + 1;

    while (seg_index != curSegId_) {
        if (seg_index == segNum_) {
            seg_index = 0;
        }
        if (segTable_[seg_index].state == SegUseStat::FREE) {
            seg_id = seg_index;
            // set seg used
            curSegId_ = seg_id;
            segTable_[curSegId_].state = SegUseStat::RESERVED;

            reservedCounter_++;
            freedCounter_--;
            return true;
        }
        seg_index++;
    }
    return false;
}

void SegmentManager::FreeForFailed(uint32_t seg_id) {
    std::lock_guard < std::mutex > l(mtx_);
    segTable_[seg_id].state = SegUseStat::FREE;
    segTable_[seg_id].free_size = 0;
    segTable_[seg_id].death_size = 0;

    reservedCounter_--;
    freedCounter_++;
    __DEBUG("Free Segment seg_id = %d", seg_id);
}

void SegmentManager::FreeForGC(uint32_t seg_id) {
    std::lock_guard < std::mutex > l(mtx_);
    segTable_[seg_id].state = SegUseStat::FREE;
    segTable_[seg_id].free_size = 0;
    segTable_[seg_id].death_size = 0;

    usedCounter_--;
    freedCounter_++;
    __DEBUG("Free Segment For GC, seg_id = %d", seg_id);
}

void SegmentManager::Use(uint32_t seg_id, uint32_t free_size) {
    std::lock_guard < std::mutex > l(mtx_);
    segTable_[seg_id].state = SegUseStat::USED;
    segTable_[seg_id].free_size = free_size;

    reservedCounter_--;
    usedCounter_++;
    __DEBUG("Used Segment seg_id = %d, free_size = %d", seg_id, free_size);
}

//void SegmentManager::Reserved(uint32_t seg_id)
//{
//    std::lock_guard<std::mutex> l(mtx_);
//    segTable_[seg_id].state = SegUseStat::RESERVED;
//}

void SegmentManager::ModifyDeathEntry(HashEntry &entry) {
    uint32_t seg_id;
    uint64_t offset = entry.GetHeaderOffsetPhy();
    if (!ComputeSegIdFromOffset(offset, seg_id)) {
        __ERROR("Compute Seg Id Wrong!!! offset = %ld", offset);
    }

    uint16_t data_size = entry.GetDataSize();
    uint32_t death_size = (uint32_t) data_size
            + (uint32_t) IndexManager::SizeOfDataHeader();

    std::lock_guard < std::mutex > l(mtx_);
    segTable_[seg_id].death_size += death_size;
}

uint32_t SegmentManager::GetTotalFreeSegs() {
    std::lock_guard < std::mutex > l(mtx_);
    return freedCounter_;
}

uint32_t SegmentManager::GetTotalUsedSegs() {
    std::lock_guard < std::mutex > l(mtx_);
    return usedCounter_;
}

void SegmentManager::SortSegsByUtils(
                                     std::multimap<uint32_t, uint32_t> &cand_map,
                                     double utils) {
    std::lock_guard < std::mutex > lck(mtx_);
    uint32_t used_size;
    uint32_t thld = (uint32_t)(segSize_ * utils);

    for (uint32_t index = 0; index < segNum_; index++) {
        if (segTable_[index].state == SegUseStat::USED) {
            used_size = segSize_ - (segTable_[index].free_size
                    + segTable_[index].death_size
                    + (uint32_t) SegmentManager::SizeOfSegOnDisk());
            if (used_size < thld) {
                cand_map.insert(std::pair<uint32_t, uint32_t>(used_size, index));
            }
        }
    } __DEBUG("There is tatal %lu segments utils under %f", cand_map.size(), utils);
}

SegmentManager::SegmentManager(BlockDevice* bdev, SuperBlockManager* sbm,
                               Options &opt) :
    dataStartOff_(0), dataEndOff_(0), segSize_(0), segSizeBit_(0), segNum_(0),
            curSegId_(0), usedCounter_(0), freedCounter_(0),
            reservedCounter_(0), maxValueLen_(0), bdev_(bdev), sbMgr_(sbm),
            options_(opt) {
}

SegmentManager::~SegmentManager() {
    segTable_.clear();
}
} //end namespace kvdb
