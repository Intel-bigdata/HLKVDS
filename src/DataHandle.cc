//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <sys/uio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <iostream>
#include "DataHandle.h"

namespace kvdb {

KVSlice::KVSlice() :
    key_(NULL), keyLength_(0), data_(NULL), dataLength_(0), digest_(NULL),
            entry_(NULL), segId_(0) {
}

KVSlice::~KVSlice() {
    if (digest_) {
        delete digest_;
    }
    if (entry_) {
        delete entry_;
    }
}

KVSlice::KVSlice(const KVSlice& toBeCopied) :
    key_(NULL), keyLength_(0), data_(NULL), dataLength_(0), digest_(NULL),
            entry_(NULL), segId_(0) {
    copy_helper(toBeCopied);
}

KVSlice& KVSlice::operator=(const KVSlice& toBeCopied) {
    copy_helper(toBeCopied);
    return *this;
}

void KVSlice::copy_helper(const KVSlice& toBeCopied) {
    keyLength_ = toBeCopied.GetKeyLen();
    dataLength_ = toBeCopied.GetDataLen();
    key_ = toBeCopied.GetKey();
    data_ = toBeCopied.GetData();
    *digest_ = *toBeCopied.digest_;
    *entry_ = *toBeCopied.entry_;
    segId_ = toBeCopied.segId_;
}

KVSlice::KVSlice(const char* key, uint32_t key_len, const char* data,
                 uint16_t data_len) :
    key_(key), keyLength_(key_len), data_(data), dataLength_(data_len),
            digest_(NULL), entry_(NULL), segId_(0) {
    computeDigest();
}

KVSlice::KVSlice(Kvdb_Digest *digest, const char* data, uint32_t data_len) :
    key_(NULL), keyLength_(0), data_(data), dataLength_(data_len),
            digest_(NULL), entry_(NULL), segId_(0) {
    digest_ = new Kvdb_Digest(*digest);
}

void KVSlice::SetKeyValue(const char* key, uint32_t key_len, const char* data,
                          uint16_t data_len) {
    keyLength_ = key_len;
    dataLength_ = data_len;
    key_ = key;
    data_ = data;

    computeDigest();
}

void KVSlice::computeDigest() {
    if (digest_) {
        delete digest_;
    }
    digest_ = new Kvdb_Digest();
    Kvdb_Key vkey(key_, keyLength_);
    KeyDigestHandle::ComputeDigest(&vkey, *digest_);
}

string KVSlice::GetKeyStr() const {
    return string(key_, keyLength_);
}

string KVSlice::GetDataStr() const {
    return string(data_, dataLength_);
}

void KVSlice::SetHashEntry(const HashEntry *hash_entry) {
    entry_ = new HashEntry(*hash_entry);
}

void KVSlice::SetSegId(uint32_t seg_id) {
    segId_ = seg_id;
}

IRequest::IRequest() :
    type(1), done_(false), stat_(ReqStat::INIT), slice_(NULL),
            slices_(new std::list<KVSlice *>), segPtr_(NULL) {
}
IRequest::IRequest(KVSlice& slice) :
    type(0), done_(false), stat_(ReqStat::INIT), slice_(&slice), slices_(NULL),
            segPtr_(NULL) {
}
IRequest::~IRequest() {
    segPtr_ = NULL;
}

void IRequest::SetWriteStat(bool stat) {
    std::lock_guard < std::mutex > l(mtx_);
    stat_ = stat ? ReqStat::SUCCESS : ReqStat::FAIL;
}

void IRequest::Wait() {
    std::unique_lock < std::mutex > l(mtx_);
    while (!done_) {
        cv_.wait(l);
    }
}

void IRequest::Signal() {
    std::unique_lock < std::mutex > l(mtx_);
    done_ = true;
    cv_.notify_one();
}

Request::Request(KVSlice& slice) :
    IRequest(slice) {
}

/*Request::Request(KVSlice& slice) {
 done_ = false;
 stat_ = ReqStat::INIT;
 slice_ = &slice;
 segPtr_ = NULL;
 }*/

/*Request::Request(const Request& toBeCopied) {
 done_ = false;
 stat_ = toBeCopied.stat_;
 slice_ = toBeCopied.slice_;
 segPtr_ = toBeCopied.segPtr_;
 }

 Request& Request::operator=(const Request& toBeCopied) {
 done_ = toBeCopied.done_;
 stat_ = toBeCopied.stat_;
 slice_ = toBeCopied.slice_;
 segPtr_ = toBeCopied.segPtr_;
 return *this;
 }*/

Request::~Request() {
}

SegmentSlice::SegmentSlice() :
    segId_(0), segMgr_(NULL), idxMgr_(NULL), bdev_(NULL), segSize_(0),
            persistTime_(KVTime()), startTime_(KVTime()), headPos_(0),
            tailPos_(0), keyNum_(0), keyAlignedNum_(0), isCompleted_(false),
            hasReq_(false), segOndisk_(NULL) {
    segOndisk_ = new SegmentOnDisk();
}

SegmentSlice::~SegmentSlice() {
    delete segOndisk_;
}

SegmentSlice::SegmentSlice(const SegmentSlice& toBeCopied) {
    segOndisk_ = new SegmentOnDisk();
    copyHelper(toBeCopied);
}

SegmentSlice& SegmentSlice::operator=(const SegmentSlice& toBeCopied) {
    copyHelper(toBeCopied);
    return *this;
}

void SegmentSlice::copyHelper(const SegmentSlice& toBeCopied) {
    segId_ = toBeCopied.segId_;
    segMgr_ = toBeCopied.segMgr_;
    idxMgr_ = toBeCopied.idxMgr_;
    bdev_ = toBeCopied.bdev_;
    timeout_ = toBeCopied.timeout_;
    segSize_ = toBeCopied.segSize_;
    persistTime_ = toBeCopied.persistTime_;
    startTime_ = toBeCopied.startTime_;
    headPos_ = toBeCopied.headPos_;
    tailPos_ = toBeCopied.tailPos_;
    keyNum_ = toBeCopied.keyNum_;
    keyAlignedNum_ = toBeCopied.keyAlignedNum_;
    isCompleted_ = toBeCopied.isCompleted_;
    hasReq_ = toBeCopied.hasReq_;
    reqCommited_.store(toBeCopied.reqCommited_.load());
    reqList_ = toBeCopied.reqList_;
    *segOndisk_ = *toBeCopied.segOndisk_;
    delReqList_ = toBeCopied.delReqList_;
}

SegmentSlice::SegmentSlice(SegmentManager* sm, IndexManager* im,
                           BlockDevice* bdev, uint32_t timeout) :
    segId_(0), segMgr_(sm), idxMgr_(im), bdev_(bdev), timeout_(timeout),
            segSize_(segMgr_->GetSegmentSize()), persistTime_(KVTime()),
            startTime_(KVTime()), headPos_(SegmentManager::SizeOfSegOnDisk()),
            tailPos_(segSize_), keyNum_(0), keyAlignedNum_(0),
            isCompleted_(false), hasReq_(false), reqCommited_(-1),
            segOndisk_(NULL) {
    segOndisk_ = new SegmentOnDisk();
}

bool SegmentSlice::TryPut(IRequest* req) {
    if (isCompleted_) {
        return false;
    }
    return isCanFit(req);
}

void SegmentSlice::Put(IRequest* req) {
    int reqType = req->getType();
    KVSlice *slice = &req->GetSlice();

    if (keyNum_ == 0) {
        hasReq_ = true;
        startTime_.Update();
    }

    if (1 == reqType) {
        std::list<KVSlice *> *slices = &req->getSlices();
        for (list<KVSlice *>::iterator iter = slices->begin(); iter
                != slices->end(); iter++) {
            KVSlice *slice = (*iter);
            if (slice->IsAlignedData()) {
                headPos_ += IndexManager::SizeOfDataHeader()
                        + slice->GetKeyLen();
                tailPos_ -= ALIGNED_SIZE;
                keyAlignedNum_++;
            } else {
                headPos_ += IndexManager::SizeOfDataHeader()
                        + slice->GetKeyLen() + slice->GetDataLen();
            }
            keyNum_++;
            __DEBUG("Put request key = %s", slice->GetKeyStr().c_str());
        }

    } else {
        if (slice->IsAlignedData()) {
            headPos_ += IndexManager::SizeOfDataHeader() + slice->GetKeyLen();
            tailPos_ -= ALIGNED_SIZE;
            keyAlignedNum_++;
        } else {
            headPos_ += IndexManager::SizeOfDataHeader() + slice->GetKeyLen()
                    + slice->GetDataLen();
        }
        keyNum_++;
        __DEBUG("Put request key = %s", slice->GetKeyStr().c_str());
    }

    reqList_.push_back(req);
    req->SetSeg(this);

}

bool SegmentSlice::_writeDataToDevice() {
    char *data_buff = new char[segSize_];
    copyToData(data_buff);

    uint64_t offset = 0;
    segMgr_->ComputeSegOffsetFromId(segId_, offset);

    bool wstat = true;
    if (bdev_->pWrite(data_buff, segSize_, offset) != segSize_) {
        __ERROR("Write Segment front data error, seg_id:%u", segId_);
        wstat = false;
    }

    delete[] data_buff;
    return wstat;
}

bool SegmentSlice::WriteSegToDevice(uint32_t seg_id) {
    segId_ = seg_id;
    fillSlice();
    __DEBUG("Begin write seg, free size %u, seg id: %d, key num: %d", tailPos_-headPos_ , segId_, keyNum_);

    return _writeDataToDevice();
}

void SegmentSlice::fillSlice() {
    uint32_t head_pos = SegmentManager::SizeOfSegOnDisk();
    uint32_t tail_pos = segSize_;
    for (list<IRequest *>::iterator iter = reqList_.begin(); iter
            != reqList_.end(); iter++) {

        int reqType = (*iter)->getType();
        if (1 == reqType) {
            list<KVSlice *> *slices = &(*iter)->getSlices();
            for (list<KVSlice *>::iterator iter2 = slices->begin(); iter2
                    != slices->end(); iter2++) {
                KVSlice *slice2 = (*iter2);
                slice2->SetSegId(segId_);
                if (slice2->IsAlignedData()) {
                    uint32_t data_offset = tail_pos - ALIGNED_SIZE;
                    uint32_t next_offset = head_pos
                            + IndexManager::SizeOfDataHeader()
                            + slice2->GetKeyLen();

                    DataHeader data_header(slice2->GetKey(),
                                           slice2->GetKeyLen(),
                                           slice2->GetDigest(),
                                           slice2->GetDataLen(), data_offset,
                                           next_offset);

                    uint64_t seg_offset = 0;
                    segMgr_->ComputeSegOffsetFromId(segId_, seg_offset);
                    uint64_t header_offset = seg_offset + head_pos;

                    HashEntry hash_entry(data_header, header_offset, NULL);
                    slice2->SetHashEntry(&hash_entry);

                    head_pos += IndexManager::SizeOfDataHeader()
                            + slice2->GetKeyLen();
                    tail_pos -= ALIGNED_SIZE;
                    __DEBUG("SegmentSlice: key=%s, data_offset=%u, header_offset=%lu, seg_id=%u, head_pos=%u, tail_pos = %u", slice2->GetKey(), data_offset, header_offset, segId_, head_pos, tail_pos);
                } else {
                    uint32_t data_offset = head_pos
                            + IndexManager::SizeOfDataHeader()
                            + slice2->GetKeyLen();
                    uint32_t next_offset = head_pos
                            + IndexManager::SizeOfDataHeader()
                            + slice2->GetKeyLen() + slice2->GetDataLen();
                    DataHeader data_header(slice2->GetKey(),
                                           slice2->GetKeyLen(),
                                           slice2->GetDigest(),
                                           slice2->GetDataLen(), data_offset,
                                           next_offset);

                    uint64_t seg_offset = 0;
                    segMgr_->ComputeSegOffsetFromId(segId_, seg_offset);
                    uint64_t header_offset = seg_offset + head_pos;

                    HashEntry hash_entry(data_header, header_offset, NULL);
                    slice2->SetHashEntry(&hash_entry);

                    head_pos += IndexManager::SizeOfDataHeader()
                            + slice2->GetKeyLen() + slice2->GetDataLen();
                    __DEBUG("SegmentSlice: key=%s, data_offset=%u, header_offset=%lu, seg_id=%u, head_pos=%u, tail_pos = %u", slice2->GetKey(), data_offset, header_offset, segId_, head_pos, tail_pos);

                }
            }
        } else {
            KVSlice *slice = &(*iter)->GetSlice();
            slice->SetSegId(segId_);
            if (slice->IsAlignedData()) {
                uint32_t data_offset = tail_pos - ALIGNED_SIZE;
                uint32_t next_offset = head_pos
                        + IndexManager::SizeOfDataHeader();
                DataHeader data_header(slice->GetKey(), slice->GetKeyLen(),
                                       slice->GetDigest(), slice->GetDataLen(),
                                       data_offset, next_offset);
                uint64_t seg_offset = 0;
                segMgr_->ComputeSegOffsetFromId(segId_, seg_offset);
                uint64_t header_offset = seg_offset + head_pos;

                HashEntry hash_entry(data_header, header_offset, NULL);
                slice->SetHashEntry(&hash_entry);

                head_pos += IndexManager::SizeOfDataHeader()
                        + slice->GetKeyLen();
                tail_pos -= ALIGNED_SIZE;
                __DEBUG("SegmentSlice: key=%s, data_offset=%u, header_offset=%lu, seg_id=%u, head_pos=%u, tail_pos = %u", slice->GetKey(), data_offset, header_offset, segId_, head_pos, tail_pos);
            } else {
                uint32_t data_offset = head_pos
                        + IndexManager::SizeOfDataHeader() + slice->GetKeyLen();
                uint32_t next_offset = head_pos
                        + IndexManager::SizeOfDataHeader() + slice->GetKeyLen()
                        + slice->GetDataLen();

                DataHeader data_header(slice->GetKey(), slice->GetKeyLen(),
                                       slice->GetDigest(), slice->GetDataLen(),
                                       data_offset, next_offset);
                uint64_t seg_offset = 0;
                segMgr_->ComputeSegOffsetFromId(segId_, seg_offset);
                uint64_t header_offset = seg_offset + head_pos;

                HashEntry hash_entry(data_header, header_offset, NULL);
                slice->SetHashEntry(&hash_entry);

                head_pos += IndexManager::SizeOfDataHeader()
                        + slice->GetKeyLen() + slice->GetDataLen();
                __DEBUG("SegmentSlice: key=%s, data_offset=%u, header_offset=%lu, seg_id=%u, head_pos=%u, tail_pos = %u", slice->GetKey(), data_offset, header_offset, segId_, head_pos, tail_pos);

            }
        }

    }
    if (head_pos != headPos_ || tail_pos != tailPos_) {
        __ERROR("Segment fillSlice  Failed!!! head_pos= %u, headPos_=%u, tail_pos= %u, tailPos_ = %u", head_pos, headPos_, tail_pos, tailPos_);
    }
}

void SegmentSlice::notifyAndClean(bool req_state) {
    std::lock_guard < std::mutex > l(mtx_);

    //Set logic timestamp to hashentry
    reqCommited_.store(keyNum_);

    int32_t keyNo = 0;
    persistTime_.Update();
    for (list<IRequest *>::iterator iter = reqList_.begin(); iter
            != reqList_.end(); iter++) {
        int reqType = (*iter)->getType();
        if (0 == reqType) {
            KVSlice *slice = &(*iter)->GetSlice();
            keyNo++;
            HashEntry &entry = slice->GetHashEntry();
            entry.SetLogicStamp(persistTime_, keyNo);

            if (!slice->GetData()) {
                delReqList_.push_back(entry);
            }
        } else {
            list<KVSlice *> *slices = &(*iter)->getSlices();
            for (list<KVSlice *>::iterator iter2 = slices->begin(); iter2
                    != slices->end(); iter2++) {
                keyNo++;
                KVSlice *slice = (*iter2);
                HashEntry &entry = slice->GetHashEntry();
                entry.SetLogicStamp(persistTime_, keyNo);

                if (!slice->GetData()) {
                    delReqList_.push_back(entry);
                }
            }
        }
    }

    //notify and clean req.
    while (!reqList_.empty()) {
        IRequest *req = reqList_.front();
        reqList_.pop_front();
        req->SetWriteStat(req_state);
        req->Signal();
    }

}

void SegmentSlice::Complete() {
    if (isCompleted_) {
        return;
    }

    fillSegHead();
    isCompleted_ = true;
}

void SegmentSlice::Notify(bool stat) {
    notifyAndClean(stat);
}

void SegmentSlice::CleanDeletedEntry() {
    std::lock_guard < std::mutex > l(mtx_);
    for (list<HashEntry>::iterator iter = delReqList_.begin(); iter
            != delReqList_.end(); iter++) {
        idxMgr_->RemoveEntry(*iter);
    }
    delReqList_.clear();
}

bool SegmentSlice::IsExpired() {
    KVTime nowTime;
    if (!hasReq_) {
        return false;
    }
    int64_t interval = nowTime - startTime_;
    return (interval > timeout_);
}

bool SegmentSlice::isCanFit(IRequest* req) const {
    uint32_t freeSize = tailPos_ - headPos_;
    uint32_t needSize = 0;

    if ((req->getType()) == 0) {
        KVSlice *slice = &req->GetSlice();
        needSize = slice->GetDataLen() + IndexManager::SizeOfDataHeader()
                + slice->GetKeyLen();
    } else {
        std::list<KVSlice *> *slices = &req->getSlices();
        for (list<KVSlice *>::iterator iter = slices->begin(); iter
                != slices->end(); iter++) {
            needSize += (*iter)->GetDataLen()
                    + IndexManager::SizeOfDataHeader() + (*iter)->GetKeyLen();
        }
    }

    return freeSize > needSize;
}

uint32_t SegmentSlice::CopyDataHeader(char* data_buff, DataHeader* header) {
    uint32_t offset_begin=0;

    uint32_t key_len = header->GetKeyLen();
    memcpy(&(data_buff[offset_begin]), header->GetKey(), key_len);
    offset_begin += key_len;

    memcpy(&(data_buff[offset_begin]), &key_len, sizeof(key_len));
    offset_begin += sizeof(key_len);

    Kvdb_Digest digest = header->GetDigest();
    memcpy(&(data_buff[offset_begin]), &digest, sizeof(digest));
    offset_begin += sizeof(digest);

    uint16_t data_size = header->GetDataSize();
    memcpy(&(data_buff[offset_begin]), &data_size, sizeof(data_size));
    offset_begin += sizeof(data_size);

    uint32_t data_offset = header->GetDataOffset();
    memcpy(&(data_buff[offset_begin]), &data_offset, sizeof(data_offset));
    offset_begin += sizeof(data_offset);

    uint32_t next_header_offset = header->GetNextHeadOffset();
    memcpy(&(data_buff[offset_begin]), &next_header_offset, sizeof(next_header_offset));
    offset_begin += sizeof(next_header_offset);

    return offset_begin;
}

void SegmentSlice::copyToData(char* data_buff) {
    uint64_t offset = 0;
    segMgr_->ComputeSegOffsetFromId(segId_, offset);

    uint32_t offset_begin = 0;
    uint32_t offset_end = segSize_;

    memcpy(data_buff, segOndisk_, SegmentManager::SizeOfSegOnDisk());
    offset_begin += SegmentManager::SizeOfSegOnDisk();

    //aggregate iovec
    for (list<IRequest *>::iterator iter = reqList_.begin(); iter
            != reqList_.end(); iter++) {
        int reqType = (*iter)->getType();
        if (0 == reqType) {
            KVSlice *slice = &(*iter)->GetSlice();
            DataHeader *header =
                    &slice->GetHashEntry().GetEntryOnDisk().GetDataHeader();
            char *data = (char *) slice->GetData();
            uint16_t data_len = slice->GetDataLen();

          /*memcpy(&(data_buff[offset_begin]), header,
                   IndexManager::SizeOfDataHeader() + slice->GetKeyLen());
            offset_begin += IndexManager::SizeOfDataHeader()
                    + slice->GetKeyLen();*/
            offset_begin+=CopyDataHeader(data_buff,header);

            if (slice->IsAlignedData()) {
                offset_end -= data_len;
                memcpy(&(data_buff[offset_end]), data, data_len);
                __DEBUG("write key = %s, data position: %lu", slice->GetKey(), offset_end + offset);
            } else {
                memcpy(&(data_buff[offset_begin]), data, data_len);
                offset_begin += data_len;
                __DEBUG("write key = %s, data position: %lu", slice->GetKey(), offset_begin + offset);
            }
        } else {
            list<KVSlice *> slices = (*iter)->getSlices();
            for (list<KVSlice *>::iterator iter2 = slices.begin(); iter2
                    != slices.end(); iter2++) {
                KVSlice *slice = (*iter2);
                DataHeader *header =
                        &slice->GetHashEntry().GetEntryOnDisk().GetDataHeader();
                char *data = (char *) slice->GetData();
                uint16_t data_len = slice->GetDataLen();

               /* memcpy(&(data_buff[offset_begin]), header,
                       IndexManager::SizeOfDataHeader() + slice->GetKeyLen());
                offset_begin += IndexManager::SizeOfDataHeader()
                        + slice->GetKeyLen();*/
                offset_begin+=CopyDataHeader(data_buff,header);

                if (slice->IsAlignedData()) {
                    offset_end -= data_len;
                    memcpy(&(data_buff[offset_end]), data, data_len);
                    __DEBUG("write key = %s, data position: %lu", slice->GetKey(), offset_end + offset);
                } else {
                    memcpy(&(data_buff[offset_begin]), data, data_len);
                    offset_begin += data_len;
                    __DEBUG("write key = %s, data position: %lu", slice->GetKey(), offset_begin + offset);
                }
            }
        }
    }
    memset(&(data_buff[offset_begin]), 0, (offset_end - offset_begin));

}

void SegmentSlice::fillSegHead() {
    segOndisk_->SetKeyNum(keyNum_);
}

}
