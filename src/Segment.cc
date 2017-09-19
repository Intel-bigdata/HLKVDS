#include <sys/uio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>

#include "Segment.h"

namespace hlkvds {

KVSlice::KVSlice() :
    key_(NULL), keyLength_(0), data_(NULL), dataLength_(0), digest_(NULL),
            entry_(NULL), segId_(0) {
}

KVSlice::~KVSlice() {
    if (deepCopy_) {
        delete[] key_;
        delete[] data_;
    }
    if (digest_) {
        delete digest_;
    }
    if (entry_) {
        delete entry_;
    }
}

KVSlice::KVSlice(const KVSlice& toBeCopied) :
    key_(NULL), keyLength_(0), data_(NULL), dataLength_(0), digest_(NULL),
            entry_(NULL), segId_(0), deepCopy_(false) {
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
    deepCopy_ = toBeCopied.deepCopy_;
}

KVSlice::KVSlice(const char* key, int key_len, const char* data, int data_len, bool deep_copy) :
    key_(NULL), keyLength_(key_len), data_(NULL), dataLength_(data_len),
            digest_(NULL), entry_(NULL), segId_(0), deepCopy_(deep_copy) {
    if (deepCopy_) {
        key_ = new char[key_len];
        data_ = new char[data_len];
        memcpy((void*)key_, key, key_len);
        memcpy((void*)data_, data, data_len);
    } else {
        key_ = key;
        data_ = data;
    }
    computeDigest();
}

#ifdef WITH_ITERATOR
KVSlice::KVSlice(Kvdb_Digest *digest, const char* key, int key_len,
                const char* data, int data_len) :
    key_(key), keyLength_(key_len), data_(data), dataLength_(data_len),
            digest_(NULL), entry_(NULL), segId_(0), deepCopy_(false) {
    digest_ = new Kvdb_Digest(*digest);
}
#else
KVSlice::KVSlice(Kvdb_Digest *digest, const char* data, int data_len) :
    key_(NULL), keyLength_(0), data_(data), dataLength_(data_len),
            digest_(NULL), entry_(NULL), segId_(0) {
    digest_ = new Kvdb_Digest(*digest);
}
#endif


void KVSlice::SetKeyValue(const char* key, int key_len, const char* data,
                          int data_len) {
    keyLength_ = key_len;
    dataLength_ = data_len;
    key_ = key;
    data_ = data;

    computeDigest();
}

void KVSlice::computeDigest() {
    if (digest_) {
        delete digest_;
        digest_=nullptr;
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

Request::Request() :
    done_(false), stat_(ReqStat::INIT), slice_(NULL), segPtr_(NULL) {
}

Request::~Request() {
}

Request::Request(const Request& toBeCopied) :
    done_(false), stat_(toBeCopied.stat_), slice_(toBeCopied.slice_),
    segPtr_(toBeCopied.segPtr_) {
}

Request& Request::operator=(const Request& toBeCopied) {
    done_ = toBeCopied.done_;
    stat_ = toBeCopied.stat_;
    slice_ = toBeCopied.slice_;
    segPtr_ = toBeCopied.segPtr_;
    return *this;
}
Request::Request(KVSlice& slice) : 
    done_(false), stat_(ReqStat::INIT), slice_(&slice), segPtr_(NULL) {
}

void Request::SetWriteStat(bool stat) {
    std::lock_guard<std::mutex> l(mtx_);
    stat_ = stat? ReqStat::SUCCESS : ReqStat::FAIL ;
}

void Request::Wait() {
    std::unique_lock<std::mutex> l(mtx_);
    while(!done_) {
        cv_.wait(l);
    }
}

void Request::Signal() {
    std::unique_lock<std::mutex> l(mtx_);
    done_ = true;
    cv_.notify_one();
}

SegBase::SegBase() :
    segId_(-1), segMgr_(NULL), bdev_(NULL), segSize_(-1),
        headPos_(0), tailPos_(0), keyNum_(0),
        keyAlignedNum_(0), segOndisk_(NULL), dataBuf_(NULL) {
    segOndisk_ = new SegmentOnDisk();
}

SegBase::~SegBase() {
    if (segOndisk_) {
        delete segOndisk_;
    }
    if (dataBuf_) {
	free(dataBuf_);
        //delete[] dataBuf_;
    }
}

SegBase::SegBase(const SegBase& toBeCopied) {
    segOndisk_ = new SegmentOnDisk();
    copyHelper(toBeCopied);
}

SegBase& SegBase::operator=(const SegBase& toBeCopied) {
    copyHelper(toBeCopied);
    return *this;
}

void SegBase::copyHelper(const SegBase& toBeCopied) {
    segId_ = toBeCopied.segId_;
    segMgr_ = toBeCopied.segMgr_;
    bdev_ = toBeCopied.bdev_;
    segSize_ = toBeCopied.segSize_;
    headPos_ = toBeCopied.headPos_;
    tailPos_ = toBeCopied.tailPos_;
    keyNum_ = toBeCopied.keyNum_;
    keyAlignedNum_ = toBeCopied.keyAlignedNum_;
    *segOndisk_ = *toBeCopied.segOndisk_;
    memcpy(dataBuf_, toBeCopied.dataBuf_, segSize_);
    sliceList_ = toBeCopied.sliceList_;
}

SegBase::SegBase(SegmentManager* sm, BlockDevice* bdev) :
    segId_(-1), segMgr_(sm), bdev_(bdev),
        segSize_(segMgr_->GetSegmentSize()),
        headPos_(SegmentManager::SizeOfSegOnDisk()), tailPos_(segSize_),
        keyNum_(0), keyAlignedNum_(0), segOndisk_(NULL), dataBuf_(NULL) {
    segOndisk_ = new SegmentOnDisk();
}

bool SegBase::TryPut(KVSlice* slice) {
    uint32_t freeSize = tailPos_ - headPos_;
#ifdef WITH_ITERATOR
    uint32_t needSize = slice->GetDataLen() + slice->GetKeyLen() + IndexManager::SizeOfDataHeader();
#else
    uint32_t needSize = slice->GetDataLen() + IndexManager::SizeOfDataHeader();
#endif
    return freeSize > needSize;
}

void SegBase::Put(KVSlice* slice) {
    if (slice->IsAlignedData()) {
#ifdef WITH_ITERATOR
        headPos_ += IndexManager::SizeOfDataHeader() + slice->GetKeyLen();
#else
        headPos_ += IndexManager::SizeOfDataHeader();
#endif
        tailPos_ -= ALIGNED_SIZE;
        keyAlignedNum_++;
    } else {
#ifdef WITH_ITERATOR
        headPos_ += IndexManager::SizeOfDataHeader() + slice->GetKeyLen() + slice->GetDataLen();
#else
        headPos_ += IndexManager::SizeOfDataHeader() + slice->GetDataLen();
#endif
    }
    keyNum_++;
    sliceList_.push_back(slice);
    __DEBUG("Put request key = %s", slice->GetKeyStr().c_str());
}

bool SegBase::WriteSegToDevice() {
    if (segId_ < 0)
    {
        __ERROR("Not set seg_id to segment");
        return false;
    }
    fillEntryToSlice();
    __DEBUG("Begin write seg, free size %u, seg id: %d, key num: %d", tailPos_-headPos_ , segId_, keyNum_);
    return _writeDataToDevice();
}

void SegBase::fillEntryToSlice() {
    uint32_t head_pos = SegmentManager::SizeOfSegOnDisk();
    uint32_t tail_pos = segSize_;
    for (list<KVSlice *>::iterator iter = sliceList_.begin(); iter
            != sliceList_.end(); iter++) {

        KVSlice *slice = *iter;
        slice->SetSegId(segId_);
        if (slice->IsAlignedData()) {
            uint32_t data_offset = tail_pos - ALIGNED_SIZE;
#ifdef WITH_ITERATOR
            uint32_t next_offset = head_pos + IndexManager::SizeOfDataHeader() + slice->GetKeyLen();
            DataHeader data_header(slice->GetDigest(), slice->GetKeyLen(), slice->GetDataLen(),
                                   data_offset, next_offset);
#else
            uint32_t next_offset = head_pos + IndexManager::SizeOfDataHeader();
            DataHeader data_header(slice->GetDigest(), slice->GetDataLen(),
                                   data_offset, next_offset);
#endif

            uint64_t seg_offset = 0;
            segMgr_->ComputeSegOffsetFromId(segId_, seg_offset);
            uint64_t header_offset = seg_offset + head_pos;

            HashEntry hash_entry(data_header, header_offset, NULL);
            slice->SetHashEntry(&hash_entry);

#ifdef WITH_ITERATOR
            head_pos += IndexManager::SizeOfDataHeader() + slice->GetKeyLen();
#else
            head_pos += IndexManager::SizeOfDataHeader();
#endif
            tail_pos -= ALIGNED_SIZE;
            __DEBUG("SegmentSlice: key=%s, data_offset=%u, header_offset=%lu, seg_id=%u, head_pos=%u, tail_pos = %u", slice->GetKey(), data_offset, header_offset, segId_, head_pos, tail_pos);
        } else {
#ifdef WITH_ITERATOR
            uint32_t data_offset = head_pos + IndexManager::SizeOfDataHeader() + slice->GetKeyLen();
            uint32_t next_offset = head_pos + IndexManager::SizeOfDataHeader()
                    + slice->GetKeyLen() + slice->GetDataLen();
#else
            uint32_t data_offset = head_pos + IndexManager::SizeOfDataHeader();
            uint32_t next_offset = head_pos + IndexManager::SizeOfDataHeader()
                    + slice->GetDataLen();

#endif

#ifdef WITH_ITERATOR
            DataHeader data_header(slice->GetDigest(), slice->GetKeyLen(), slice->GetDataLen(),
                                   data_offset, next_offset);
#else
            DataHeader data_header(slice->GetDigest(), slice->GetDataLen(),
                                   data_offset, next_offset);
#endif
            uint64_t seg_offset = 0;
            segMgr_->ComputeSegOffsetFromId(segId_, seg_offset);
            uint64_t header_offset = seg_offset + head_pos;

            HashEntry hash_entry(data_header, header_offset, NULL);
            slice->SetHashEntry(&hash_entry);

#ifdef WITH_ITERATOR
            head_pos += IndexManager::SizeOfDataHeader() + slice->GetKeyLen() + slice->GetDataLen();
#else
            head_pos += IndexManager::SizeOfDataHeader() + slice->GetDataLen();
#endif
            __DEBUG("SegmentSlice: key=%s, data_offset=%u, header_offset=%lu, seg_id=%u, head_pos=%u, tail_pos = %u", slice->GetKey(), data_offset, header_offset, segId_, head_pos, tail_pos);

        }
    }
    if (head_pos != headPos_ || tail_pos != tailPos_) {
        __ERROR("Segment fillEntryToSlice  Failed!!! head_pos= %u, headPos_=%u, tail_pos= %u, tailPos_ = %u", head_pos, headPos_, tail_pos, tailPos_);
    }
}


bool SegBase::_writeDataToDevice() {
    if (!newDataBuffer()) {
        __ERROR("Write Segment error cause by cann't alloc memory, seg_id:%u", segId_);
        return false;
    }

    copyToDataBuf();
    uint64_t offset = 0;
    segMgr_->ComputeSegOffsetFromId(segId_, offset);

    if (bdev_->pWrite(dataBuf_, segSize_, offset) != segSize_) {
        __ERROR("Write Segment front data error, seg_id:%u", segId_);
        return false;
    }
    return true;
}

bool SegBase::newDataBuffer() {
    //dataBuf_ = new char[segSize_];
    posix_memalign((void **)&dataBuf_, 4096, segSize_);
    return true;
}

void SegBase::copyToDataBuf() {
    uint64_t offset = 0;
    segMgr_->ComputeSegOffsetFromId(segId_, offset);

    uint32_t offset_begin = 0;
    uint32_t offset_end = segSize_;

    offset_begin += SegmentManager::SizeOfSegOnDisk();

    //aggregate iovec
    for (list<KVSlice *>::iterator iter = sliceList_.begin(); iter
            != sliceList_.end(); iter++) {
        KVSlice *slice = *iter;
        DataHeader *header =
                &slice->GetHashEntry().GetEntryOnDisk().GetDataHeader();
        char *data = (char *) slice->GetData();
        uint16_t data_len = slice->GetDataLen();

#ifdef WITH_ITERATOR
        char *key = (char *) slice->GetKey();
        uint16_t key_len = slice->GetKeyLen();
#else
#endif
        memcpy(&(dataBuf_[offset_begin]), header,
               IndexManager::SizeOfDataHeader());
        offset_begin += IndexManager::SizeOfDataHeader();
#ifdef WITH_ITERATOR
        memcpy(&(dataBuf_[offset_begin]), key, key_len);
        offset_begin += key_len;
#else
#endif

        if (slice->IsAlignedData()) {
            offset_end -= data_len;
            memcpy(&(dataBuf_[offset_end]), data, data_len);
            __DEBUG("write key = %s, data position: %u", slice->GetKey(), offset_end);
        } else {
            memcpy(&(dataBuf_[offset_begin]), data, data_len);
            __DEBUG("write key = %s, data position: %u", slice->GetKey(), offset_begin);
            offset_begin += data_len;
        }
    }

    //copy segment header to data buffer.
    //segOndisk_->SetTS(persistTime_);
    segOndisk_->SetKeyNum(keyNum_);
    //setOndisk_->SetCrc(crc_num);
    memcpy(dataBuf_, segOndisk_, SegmentManager::SizeOfSegOnDisk());

    //set 0 to free data buffer
    memset(&(dataBuf_[offset_begin]), 0, (offset_end - offset_begin));
}

SegForReq::SegForReq() :
    SegBase(), idxMgr_(NULL), timeout_(0), startTime_(KVTime()), persistTime_(KVTime()),
        isCompleted_(false), hasReq_(false), reqCommited_(0) {
}

SegForReq::~SegForReq() {
}

SegForReq::SegForReq(const SegForReq& toBeCopied) : SegBase(toBeCopied) {
}

SegForReq& SegForReq::operator=(const SegForReq& toBeCopied) {
    if (this == &toBeCopied) {
        return *this;
    }

    SegBase::operator=(toBeCopied);
    idxMgr_ = toBeCopied.idxMgr_;
    timeout_ = toBeCopied.timeout_;
    startTime_ = toBeCopied.startTime_;
    persistTime_ = toBeCopied.persistTime_;
    isCompleted_ = toBeCopied.isCompleted_;
    hasReq_ = toBeCopied.hasReq_;
    reqCommited_.store(toBeCopied.reqCommited_.load());
    reqList_ = toBeCopied.reqList_;
    delReqList_ = toBeCopied.delReqList_;
    return *this;
}

SegForReq::SegForReq(SegmentManager* sm, IndexManager* im, BlockDevice* bdev, uint32_t timeout) :
    SegBase(sm, bdev), idxMgr_(im), timeout_(timeout), startTime_(KVTime()), persistTime_(KVTime()),
    isCompleted_(false), hasReq_(false), reqCommited_(0) {
}

bool SegForReq::TryPut(Request* req) {
    if (isCompleted_) {
        return false;
    }
    KVSlice *slice= &req->GetSlice();
    return SegBase::TryPut(slice);
}

void SegForReq::Put(Request* req) {
    KVSlice *slice = &req->GetSlice();
    if (GetKeyNum() == 0) {
        hasReq_ = true;
        startTime_.Update();
    }
    SegBase::Put(slice);
    reqList_.push_back(req);
    req->SetSeg(this);
    __DEBUG("Put request key = %s", req->GetSlice().GetKeyStr().c_str());
}

void SegForReq::Complete() {
    if (isCompleted_) {
         return;
    }
    isCompleted_ = true;
}

void SegForReq::Notify(bool stat) {
    std::lock_guard < std::mutex > l(mtx_);

    //Set logic timestamp to hashentry
    reqCommited_.store(GetKeyNum());

    int32_t keyNo = 0;
    persistTime_.Update();
    for (list<Request *>::iterator iter = reqList_.begin(); iter
            != reqList_.end(); iter++) {
        keyNo++;
        KVSlice *slice = &(*iter)->GetSlice();
        HashEntry &entry = slice->GetHashEntry();
        entry.SetLogicStamp(persistTime_, keyNo);

        if (!slice->GetData()) {
            delReqList_.push_back(entry);
        }
    }

    //notify and clean req.
    while (!reqList_.empty()) {
        Request *req = reqList_.front();
        reqList_.pop_front();
        req->SetWriteStat(stat);
        req->Signal();
    }

}

bool SegForReq::IsExpired() {
    KVTime nowTime;
    if (!hasReq_) {
        return false;
    }
    int64_t interval = nowTime - startTime_;
    return (interval > timeout_);
}

void SegForReq::CleanDeletedEntry() {
    std::lock_guard < std::mutex > l(mtx_);
    for (list<HashEntry>::iterator iter = delReqList_.begin(); iter
            != delReqList_.end(); iter++) {
        idxMgr_->RemoveEntry(*iter);
    }
    delReqList_.clear();
}

SegForSlice::SegForSlice() :
    SegBase(), idxMgr_(NULL) {
}

SegForSlice::~SegForSlice() {
}

SegForSlice::SegForSlice(const SegForSlice& toBeCopied) : SegBase(toBeCopied) {
    idxMgr_ = toBeCopied.idxMgr_;
}

SegForSlice& SegForSlice::operator=(const SegForSlice& toBeCopied) {
    if (this == &toBeCopied) {
        return *this;
    }
    SegBase::operator=(toBeCopied);
    idxMgr_ = toBeCopied.idxMgr_;
    return *this;
}

SegForSlice::SegForSlice(SegmentManager* sm, IndexManager* im, BlockDevice* bdev) :
    SegBase(sm, bdev), idxMgr_(im) {
}

void SegForSlice::UpdateToIndex() {
    //std::lock_guard <std::mutex> l(mtx_);
    list<KVSlice *> &slice_list = GetSliceList();
    idxMgr_->UpdateIndexes(slice_list);
    //for (list<KVSlice *>::iterator iter = slice_list.begin(); iter
    //        != slice_list.end(); iter++) {
    //    KVSlice *slice = *iter;
    //    idxMgr_->UpdateIndex(slice);
    //} __DEBUG("UpdateToIndex Success!");

}

}
