//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include "GcManager.h"

namespace kvdb {
GcSegment::GcSegment() :
    segId_(0), segMgr_(NULL), idxMgr_(NULL), bdev_(NULL), segSize_(0),
            persistTime_(KVTime()), headPos_(0), tailPos_(0), keyNum_(0),
            keyAlignedNum_(0), segOndisk_(NULL) {
    segOndisk_ = new SegmentOnDisk();
    dataBuf_ = new char[segSize_];
}

GcSegment::~GcSegment() {
    delete segOndisk_;
    delete[] dataBuf_;
}

GcSegment::GcSegment(const GcSegment& toBeCopied) {
    segOndisk_ = new SegmentOnDisk();
    dataBuf_ = new char[segSize_];
    copyHelper(toBeCopied);
}

GcSegment& GcSegment::operator=(const GcSegment& toBeCopied) {
    copyHelper(toBeCopied);
    return *this;
}

void GcSegment::copyHelper(const GcSegment& toBeCopied) {
    segId_ = toBeCopied.segId_;
    segMgr_ = toBeCopied.segMgr_;
    idxMgr_ = toBeCopied.idxMgr_;
    bdev_ = toBeCopied.bdev_;
    segSize_ = toBeCopied.segSize_;
    persistTime_ = toBeCopied.persistTime_;
    headPos_ = toBeCopied.headPos_;
    tailPos_ = toBeCopied.tailPos_;
    keyNum_ = toBeCopied.keyNum_;
    keyAlignedNum_ = toBeCopied.keyAlignedNum_;
    *segOndisk_ = *toBeCopied.segOndisk_;
    memcpy(dataBuf_, toBeCopied.dataBuf_, segSize_);
}

GcSegment::GcSegment(SegmentManager* sm, IndexManager* im, BlockDevice* bdev) :
    segId_(0), segMgr_(sm), idxMgr_(im), bdev_(bdev),
            segSize_(segMgr_->GetSegmentSize()), persistTime_(KVTime()),
            headPos_(SegmentManager::SizeOfSegOnDisk()), tailPos_(segSize_),
            keyNum_(0), keyAlignedNum_(0), segOndisk_(NULL) {
    segOndisk_ = new SegmentOnDisk();
    dataBuf_ = new char[segSize_];
}

bool GcSegment::TryPut(KVSlice* slice) {
    return isCanFit(slice);
}

void GcSegment::Put(KVSlice* slice) {
    if (slice->IsAlignedData()) {
        headPos_ += IndexManager::SizeOfDataHeader();
        tailPos_ -= ALIGNED_SIZE;
        keyAlignedNum_++;
    } else {
        headPos_ += IndexManager::SizeOfDataHeader() + slice->GetDataLen();
    }
    keyNum_++;
    sliceList_.push_back(slice);

    __DEBUG("Put request key = %s", slice->GetKeyStr().c_str());
}

bool GcSegment::WriteSegToDevice(uint32_t seg_id) {
    segId_ = seg_id;
    fillSlice();
    __DEBUG("Begin write seg, free size %u, seg id: %d, key num: %d", tailPos_-headPos_ , segId_, keyNum_);

    return _writeDataToDevice();
}

void GcSegment::UpdateToIndex() {
    for (list<KVSlice *>::iterator iter = sliceList_.begin(); iter
            != sliceList_.end(); iter++) {
        KVSlice *slice = *iter;
        idxMgr_->UpdateIndex(slice);
    } __DEBUG("UpdateToIndex Success!");
}

bool GcSegment::isCanFit(KVSlice* slice) const {
    uint32_t freeSize = tailPos_ - headPos_;
    uint32_t needSize = slice->GetDataLen() + IndexManager::SizeOfDataHeader();
    return freeSize > needSize;
}

void GcSegment::fillSlice() {
    uint32_t head_pos = SegmentManager::SizeOfSegOnDisk();
    uint32_t tail_pos = segSize_;
    for (list<KVSlice *>::iterator iter = sliceList_.begin(); iter
            != sliceList_.end(); iter++) {

        KVSlice *slice = *iter;
        slice->SetSegId(segId_);
        if (slice->IsAlignedData()) {
            uint32_t data_offset = tail_pos - ALIGNED_SIZE;
            uint32_t next_offset = head_pos + IndexManager::SizeOfDataHeader();

            DataHeader data_header(slice->GetDigest(), slice->GetDataLen(),
                                   data_offset, next_offset);

            uint64_t seg_offset = 0;
            segMgr_->ComputeSegOffsetFromId(segId_, seg_offset);
            uint64_t header_offset = seg_offset + head_pos;

            HashEntry hash_entry(data_header, header_offset, NULL);
            slice->SetHashEntry(&hash_entry);

            head_pos += IndexManager::SizeOfDataHeader();
            tail_pos -= ALIGNED_SIZE;
            __DEBUG("SegmentSlice: key=%s, data_offset=%u, header_offset=%lu, seg_id=%u, head_pos=%u, tail_pos = %u", slice->GetKey(), data_offset, header_offset, segId_, head_pos, tail_pos);
        } else {
            uint32_t data_offset = head_pos + IndexManager::SizeOfDataHeader();
            uint32_t next_offset = head_pos + IndexManager::SizeOfDataHeader()
                    + slice->GetDataLen();

            DataHeader data_header(slice->GetDigest(), slice->GetDataLen(),
                                   data_offset, next_offset);

            uint64_t seg_offset = 0;
            segMgr_->ComputeSegOffsetFromId(segId_, seg_offset);
            uint64_t header_offset = seg_offset + head_pos;

            HashEntry hash_entry(data_header, header_offset, NULL);
            slice->SetHashEntry(&hash_entry);

            head_pos += IndexManager::SizeOfDataHeader() + slice->GetDataLen();
            __DEBUG("SegmentSlice: key=%s, data_offset=%u, header_offset=%lu, seg_id=%u, head_pos=%u, tail_pos = %u", slice->GetKey(), data_offset, header_offset, segId_, head_pos, tail_pos);

        }
    }
    if (head_pos != headPos_ || tail_pos != tailPos_) {
        __ERROR("Segment fillSlice  Failed!!! head_pos= %u, headPos_=%u, tail_pos= %u, tailPos_ = %u", head_pos, headPos_, tail_pos, tailPos_);
    }
}

bool GcSegment::_writeDataToDevice() {
    copyToData();

    uint64_t offset = 0;
    segMgr_->ComputeSegOffsetFromId(segId_, offset);

    if (bdev_->pWrite(dataBuf_, segSize_, offset) != segSize_) {
        __ERROR("Write Segment front data error, seg_id:%u", segId_);
        return false;
    }
    return true;
}

void GcSegment::copyToData() {
    uint64_t offset = 0;
    segMgr_->ComputeSegOffsetFromId(segId_, offset);

    uint32_t offset_begin = 0;
    uint32_t offset_end = segSize_;

    memcpy(dataBuf_, segOndisk_, SegmentManager::SizeOfSegOnDisk());
    offset_begin += SegmentManager::SizeOfSegOnDisk();

    //aggregate iovec
    for (list<KVSlice *>::iterator iter = sliceList_.begin(); iter
            != sliceList_.end(); iter++) {
        KVSlice *slice = *iter;
        DataHeader *header =
                &slice->GetHashEntry().GetEntryOnDisk().GetDataHeader();
        char *data = (char *) slice->GetData();
        uint16_t data_len = slice->GetDataLen();

        memcpy(&(dataBuf_[offset_begin]), header,
               IndexManager::SizeOfDataHeader());
        offset_begin += IndexManager::SizeOfDataHeader();

        if (slice->IsAlignedData()) {
            offset_end -= data_len;
            memcpy(&(dataBuf_[offset_end]), data, data_len);
            __DEBUG("write key = %s, data position: %lu", slice->GetKey(), offset_end + offset);
        } else {
            memcpy(&(dataBuf_[offset_begin]), data, data_len);
            offset_begin += data_len;
            __DEBUG("write key = %s, data position: %lu", slice->GetKey(), offset_begin + offset);
        }
    }
    memset(&(dataBuf_[offset_begin]), 0, (offset_end - offset_begin));
}

GcManager::~GcManager() {
    if (dataBuf_) {
        delete[] dataBuf_;
    }
}

GcManager::GcManager(BlockDevice* bdev, IndexManager* im, SegmentManager* sm,
                     Options &opt) :
    options_(opt), dataBuf_(NULL) {
    bdev_ = bdev;
    idxMgr_ = im;
    segMgr_ = sm;
}

bool GcManager::ForeGC() {
    std::lock_guard < std::mutex > lck_gc(gcMtx_);
    //check free segment;
    uint32_t free_seg_num = segMgr_->GetTotalFreeSegs();
    if (free_seg_num > SEG_RESERVED_FOR_GC) {
        __DEBUG("some thread already called doForeGC, ignore this call!");
        return true;
    }

    free_seg_num = 0;
    __DEBUG("Begin Fore GC !");
    while (!free_seg_num) {
        std::multimap < uint32_t, uint32_t > cands_map;
        segMgr_->SortSegsByUtils(cands_map, options_.seg_full_rate);
        if (cands_map.empty()) {
            break;
        }
        free_seg_num = doMerge(cands_map);
        cands_map.clear();
        __DEBUG("Once merge total free %d segments!", free_seg_num);
    }
    return free_seg_num ? true : false;
}

void GcManager::BackGC() {
    std::unique_lock < std::mutex > lck_gc(gcMtx_, std::defer_lock);

    uint32_t seg_size = segMgr_->GetSegmentSize();
    uint64_t data_theory_size = idxMgr_->GetDataTheorySize();
    uint32_t theory_seg_num = data_theory_size / seg_size + 1;

    uint32_t free_seg_num = segMgr_->GetTotalFreeSegs();
    uint32_t total_seg_num = segMgr_->GetNumberOfSeg();
    uint32_t used_seg_num = total_seg_num - free_seg_num;
    if (used_seg_num == 0 || theory_seg_num > used_seg_num) {
        return;
    }
    double waste_rate = 1 - ((double) theory_seg_num / (double) used_seg_num);

    __DEBUG("Begin do Background GC! waste_rate is %f, data_theory_size = %lu", waste_rate, data_theory_size);
    if (waste_rate > options_.gc_upper_level) {
        while (waste_rate > options_.gc_lower_level && used_seg_num > 1) {
            lck_gc.lock();
            std::multimap < uint32_t, uint32_t > cands_map;
            segMgr_->SortSegsByUtils(cands_map, options_.seg_full_rate);

            //TODO: Maybe some bug here!!!!!
            if (cands_map.empty() || theory_seg_num > used_seg_num) {
                lck_gc.unlock();
                break;
            }
            uint32_t total_free = doMerge(cands_map);
            cands_map.clear();

            data_theory_size = idxMgr_->GetDataTheorySize();
            theory_seg_num = data_theory_size / seg_size + 1;

            used_seg_num = segMgr_->GetTotalUsedSegs();

            waste_rate = 1 - ((double) theory_seg_num / (double) used_seg_num);
            __DEBUG("done once backgroud GC, total free %u segments, data_theory_size = %lu, theory_seg_num = %u, used_seg_num = %u, seg_size = %u, waste_rate is %f",total_free, data_theory_size, theory_seg_num, used_seg_num, seg_size, waste_rate);
            lck_gc.unlock();
        } __DEBUG("waste_rate lower than the GC_LOWER_LEVEL, end Background GC! waste_rate = %f", waste_rate);
    } __DEBUG("Finsh do Background GC! waste_rate is %f", waste_rate);
}

void GcManager::FullGC() {
    std::unique_lock < std::mutex > lck_gc(gcMtx_, std::defer_lock);

    __DEBUG("Begin do Full GC!");
    uint32_t free_before = segMgr_->GetTotalFreeSegs();
    uint32_t free_after = 0;
    uint32_t used_total = 0;

    uint32_t seg_num = segMgr_->GetNumberOfSeg();
    uint32_t total_free = 0;
    uint64_t theory_size = 0;
    uint32_t free_seg_num = 0;
    uint32_t used_seg_num = 0;
    uint64_t used_size = 0;

    while (true) {
        lck_gc.lock();
        std::multimap < uint32_t, uint32_t > cands_map;
        segMgr_->SortSegsByUtils(cands_map, options_.seg_full_rate);
        if (cands_map.size() < SEG_RESERVED_FOR_GC) {
            free_after = segMgr_->GetTotalFreeSegs();
            used_total = segMgr_->GetNumberOfSeg() - free_after;
            __DEBUG("End do Full GC! befor have %u segment free, after do full GC now have %u segments free, now used %u segments!", free_before, free_after, used_total);
            break;
        }

        total_free = doMerge(cands_map);
        theory_size = idxMgr_->GetDataTheorySize();
        free_seg_num = segMgr_->GetTotalFreeSegs();
        used_seg_num = seg_num - free_seg_num;
        used_size = (uint64_t) segMgr_->GetSegmentSize()
                * (uint64_t) used_seg_num;

        __DEBUG("complete once merge, and this merge free %u segments, theory_size is %lu Byte, used Segments total is %u, occur space is %lu, total seg is %u, free seg is %u", total_free, theory_size, used_seg_num, used_size, seg_num, free_seg_num);
        cands_map.clear();
        lck_gc.unlock();
    } __DEBUG("End do Full GC! total free %u segments", free_after - free_before);
}

uint32_t GcManager::doMerge(std::multimap<uint32_t, uint32_t> &cands_map) {
    if (!dataBuf_) {
        uint32_t seg_size = segMgr_->GetSegmentSize();
        dataBuf_ = new char[seg_size];
    }

    bool ret;
    std::vector < uint32_t > free_seg_vec;
    uint32_t last_seg_id = 0;
    bool need_flag = false;
    uint32_t total_free = 0;

    std::list<KVSlice*> slice_list;
    std::list<KVSlice*> recycle_list;

    std::list<KVSlice*>::iterator slice_iter;

    //handle first segment
    GcSegment *seg_first = new GcSegment(segMgr_, idxMgr_, bdev_);
    for (std::multimap<uint32_t, uint32_t>::iterator iter = cands_map.begin(); iter
            != cands_map.end(); ++iter) {
        uint32_t seg_id = iter->second;
        ret = loadKvList(seg_id, slice_list);
        if (!ret) {
            continue;
        }

        for (slice_iter = slice_list.begin(); slice_iter != slice_list.end();) {
            KVSlice* slice = *slice_iter;
            if (seg_first->TryPut(slice)) {
                seg_first->Put(slice);
                recycle_list.push_back(*slice_iter);
                slice_list.erase(slice_iter++);
            } else {
                need_flag = true;
                break;
            }
        }

        if (need_flag) {
            last_seg_id = seg_id;
            break;
        } else {
            free_seg_vec.push_back(seg_id);
        }
    }

    uint32_t seg_first_id;
    segMgr_->AllocForGC(seg_first_id);
    ret = seg_first->WriteSegToDevice(seg_first_id);
    if (!ret) {
        __ERROR("Write First GC segment to device failed, free 0 segments");
        delete seg_first;
        segMgr_->FreeForFailed(seg_first_id);
        cleanKvList(recycle_list);
        cleanKvList(slice_list);
        return total_free;
    }

    uint32_t free_size = seg_first->GetFreeSize();
    segMgr_->Use(seg_first_id, free_size);

    seg_first->UpdateToIndex();

    //clean work for first segment
    for (std::vector<uint32_t>::iterator iter = free_seg_vec.begin(); iter
            != free_seg_vec.end(); ++iter) {
        segMgr_->FreeForGC(*iter);
    }
    delete seg_first;
    cleanKvList(recycle_list);

    total_free = free_seg_vec.size() - 1;

    if (!need_flag) {
        __DEBUG("All fragment segment merge to 1 segment, and free %d segments", total_free);
        return total_free;
    }

    //handle second segment
    GcSegment *seg_second = new GcSegment(segMgr_, idxMgr_, bdev_);
    uint32_t seg_second_id;
    segMgr_->AllocForGC(seg_second_id);

    for (slice_iter = slice_list.begin(); slice_iter != slice_list.end(); ++slice_iter) {
        KVSlice* slice = *slice_iter;
        seg_second->Put(slice);
    }

    ret = seg_second->WriteSegToDevice(seg_second_id);
    if (!ret) {
        __ERROR("Write Second GC segment to device failed, but First Segment is completed, free %d segments", total_free);
        delete seg_second;
        segMgr_->FreeForFailed(seg_second_id);
        cleanKvList(slice_list);
        return total_free;
    }

    free_size = seg_second->GetFreeSize();
    segMgr_->Use(seg_second_id, free_size);

    seg_second->UpdateToIndex();
    segMgr_->FreeForGC(last_seg_id);

    //clean work for second segment
    delete seg_second;
    cleanKvList(slice_list);

    __DEBUG("Multiple fragment segments merge to 2 segment, and free %d segments", total_free);

    return total_free;
}

bool GcManager::readSegment(uint64_t seg_offset) {
    uint32_t seg_size = segMgr_->GetSegmentSize();
    if (bdev_->pRead(dataBuf_, seg_size, seg_offset) != seg_size) {
        __ERROR("GC read segment data error!!!");
        return false;
    }
    return true;
}

void GcManager::loadSegKV(list<KVSlice*> &slice_list, uint32_t num_keys,
                          uint64_t phy_offset) {
    uint32_t head_offset = SegmentManager::SizeOfSegOnDisk();

    for (uint32_t index = 0; index < num_keys; index++) {
        DataHeader header;
        memcpy(&header, &dataBuf_[head_offset],
               IndexManager::SizeOfDataHeader());

        HashEntry hash_entry(header, phy_offset + (uint64_t) head_offset, NULL);
        __DEBUG("load hash_entry from seg_offset = %ld, header_offset = %d", phy_offset, head_offset );

        if (idxMgr_->IsSameInMem(hash_entry)) {

            Kvdb_Digest digest = header.GetDigest();
            uint16_t data_len = header.GetDataSize();
            if (data_len != 0) {
                uint32_t data_offset = header.GetDataOffset();
                char* data = new char[data_len];
                memcpy(data, &dataBuf_[data_offset], data_len);

                KVSlice *slice = new KVSlice(&digest, data, data_len);
                slice_list.push_back(slice);

                __DEBUG("the slice key_digest = %s, value = %s, seg_offset = %ld, head_offset = %d is valid, need to write", digest.GetDigest(), data, phy_offset, head_offset);
            }
        }

        head_offset = header.GetNextHeadOffset();
    }

}

bool GcManager::loadKvList(uint32_t seg_id, std::list<KVSlice*> &slice_list) {
    uint64_t seg_phy_off;
    segMgr_->ComputeSegOffsetFromId(seg_id, seg_phy_off);

    if (!readSegment(seg_phy_off)) {
        return false;
    }

    SegmentOnDisk seg_disk;
    memcpy(&seg_disk, dataBuf_, SegmentManager::SizeOfSegOnDisk());

    uint32_t num_keys = seg_disk.number_keys;

    loadSegKV(slice_list, num_keys, seg_phy_off);
    return true;
}

void GcManager::cleanKvList(std::list<KVSlice*> &slice_list) {
    while (!slice_list.empty()) {
        KVSlice *slice = slice_list.front();
        slice_list.pop_front();
        const char* data = slice->GetData();
        delete[] data;
        delete slice;
    }
}

}//namespace kvdb
