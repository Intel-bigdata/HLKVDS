//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>

#include "IndexManager.h"

namespace kvdb {

DataHeader::DataHeader() :
    key_digest(Kvdb_Digest()), data_size(0), data_offset(0),
            next_header_offset(0) {
}

DataHeader::DataHeader(const Kvdb_Digest &digest, uint16_t size,
                       uint32_t offset, uint32_t next_offset) :
    key_digest(digest), data_size(size), data_offset(offset),
            next_header_offset(next_offset) {
}

DataHeader::~DataHeader() {
}

void DataHeader::SetDigest(const Kvdb_Digest& digest) {
    key_digest = digest;
}

DataHeaderOffset::~DataHeaderOffset() {
}

HashEntryOnDisk::HashEntryOnDisk() :
    header(DataHeader()), header_offset(DataHeaderOffset()) {
}

HashEntryOnDisk::HashEntryOnDisk(DataHeader& dataheader,
                                 DataHeaderOffset& offset) :
    header(dataheader), header_offset(offset) {
}

HashEntryOnDisk::HashEntryOnDisk(DataHeader& dataheader, uint64_t offset) {
    header = dataheader;
    DataHeaderOffset h_off(offset);
    header_offset = h_off;
    return;
}

HashEntryOnDisk::HashEntryOnDisk(const HashEntryOnDisk& toBeCopied) {
    header = toBeCopied.header;
    header_offset = toBeCopied.header_offset;
}

HashEntryOnDisk::~HashEntryOnDisk() {
}

HashEntryOnDisk& HashEntryOnDisk::operator=(const HashEntryOnDisk& toBeCopied) {
    header = toBeCopied.header;
    header_offset = toBeCopied.header_offset;
    return *this;
}

void HashEntryOnDisk::SetKeyDigest(const Kvdb_Digest& digest) {
    header.SetDigest(digest);
}

HashEntry::HashEntry() :
    cachePtr_(NULL) {
    stampPtr_ = new LogicStamp;
    entryPtr_ = new HashEntryOnDisk;
}

HashEntry::HashEntry(HashEntryOnDisk& entry_ondisk, KVTime time_stamp,
                     void* read_ptr) :
    cachePtr_(read_ptr) {
    stampPtr_ = new LogicStamp(time_stamp, 0);
    entryPtr_ = new HashEntryOnDisk(entry_ondisk);
}

HashEntry::HashEntry(DataHeader& data_header, uint64_t header_offset,
                     void* read_ptr) {
    stampPtr_ = new LogicStamp;
    entryPtr_ = new HashEntryOnDisk(data_header, header_offset);
    cachePtr_ = read_ptr;
}

HashEntry::HashEntry(const HashEntry& toBeCopied) {
    entryPtr_ = new HashEntryOnDisk(*toBeCopied.entryPtr_);
    stampPtr_ = new LogicStamp(*toBeCopied.stampPtr_);
    cachePtr_ = toBeCopied.cachePtr_;
}

HashEntry::~HashEntry() {
    cachePtr_ = NULL;
    if (entryPtr_) {
        delete entryPtr_;
    }
    if (stampPtr_) {
        delete stampPtr_;
    }
    return;
}

bool HashEntry::operator==(const HashEntry& toBeCompare) const {
    if (this->GetKeyDigest() == toBeCompare.GetKeyDigest()) {
        return true;
    }
    return false;
}

HashEntry& HashEntry::operator=(const HashEntry& toBeCopied) {
    *entryPtr_ = *toBeCopied.entryPtr_;
    *stampPtr_ = *toBeCopied.stampPtr_;
    cachePtr_ = toBeCopied.cachePtr_;
    return *this;
}

void HashEntry::SetKeyDigest(const Kvdb_Digest& digest) {
    entryPtr_->SetKeyDigest(digest);
}

void HashEntry::SetLogicStamp(KVTime seg_time, int32_t seg_key_no) {
    stampPtr_->Set(seg_time, seg_key_no);
}

bool IndexManager::InitIndexForCreateDB(uint64_t offset, uint32_t numObjects) {
    htSize_ = ComputeHashSizeForPower2(numObjects);
    keyCounter_ = 0;
    startOff_ = offset;

    if (!initHashTable(htSize_)) {
        return false;
    }

    //Update data theory size from superblock
    dataTheorySize_ = 0;

    return true;
}

bool IndexManager::LoadIndexFromDevice(uint64_t offset, uint32_t ht_size) {
    htSize_ = ht_size;
    startOff_ = offset;

    int64_t timeLength = KVTime::SizeOf();
    if (!rebuildTime(offset)) {
        return false;
    } __DEBUG("Load Hashtable timestamp: %s", KVTime::ToChar(*lastTime_));
    offset += timeLength;

    if (!rebuildHashTable(offset)) {
        return false;
    } __DEBUG("Rebuild Hashtable Success");

    //Update data theory size from superblock
    dataTheorySize_ = sbMgr_->GetDataTheorySize();
    uint32_t key_num = sbMgr_->GetElementNum();
    if (key_num != keyCounter_) {
        __ERROR("The Key Number is conflit between superblock and index!!!!!");
        return false;
    }

    return true;
}

bool IndexManager::rebuildTime(uint64_t offset) {
    int64_t timeLength = KVTime::SizeOf();
    time_t _time;
    if (bdev_->pRead(&_time, timeLength, offset) != timeLength) {
        __ERROR("Error in reading timestamp from file\n");
        return false;
    }
    lastTime_->SetTime(_time);
    return true;
}

bool IndexManager::WriteIndexToDevice() {
    uint64_t offset = startOff_;

    int64_t timeLength = KVTime::SizeOf();
    if (!persistTime(offset)) {
        return false;
    } __DEBUG("Write Hashtable timestamp: %s", KVTime::ToChar(*lastTime_));
    offset += timeLength;

    if (!persistHashTable(offset)) {
        return false;
    } __DEBUG("Persist Hashtable Success");

    //Update data theory size to superblock
    sbMgr_->SetDataTheorySize(dataTheorySize_);
    sbMgr_->SetElementNum(keyCounter_);

    return true;
}

bool IndexManager::persistTime(uint64_t offset) {
    int64_t timeLength = KVTime::SizeOf();
    lastTime_->Update();
    time_t _time = lastTime_->GetTime();

    if (bdev_->pWrite((void *) &_time, timeLength, offset) != timeLength) {
        __ERROR("Error write timestamp to file\n");
        return false;
    }
    return true;
}

bool IndexManager::UpdateIndex(KVSlice* slice) {
    const Kvdb_Digest *digest = &slice->GetDigest();

    HashEntry entry = slice->GetHashEntry();
    const char* data = slice->GetData();

    std::lock_guard < std::mutex > l(mtx_);

    uint32_t hash_index = KeyDigestHandle::Hash(digest) % htSize_;
    createListIfNotExist(hash_index);
    LinkedList<HashEntry> *entry_list = hashtable_[hash_index];

    bool is_exist = entry_list->search(entry);
    if (!is_exist) {
        if (data) {
            //It's insert a new entry operation
            if (keyCounter_ == htSize_) {
                __WARN("UpdateIndex Failed, because the hashtable is full!");
                return false;
            }

            entry_list->put(entry);
            keyCounter_++;
            dataTheorySize_ += SizeOfDataHeader() + slice->GetDataLen();
            __DEBUG("UpdateIndex request, because this entry is not exist! Now dataTheorySize_ is %ld", dataTheorySize_);
        } else {
            //It's a invalid delete operation
            segMgr_->ModifyDeathEntry(entry);
            __DEBUG("Ignore the UpdateIndex request, because this is a delete operation but not exist in Memory");
        }
    } else {
        HashEntry *entry_inMem = entry_list->getRef(entry);
        HashEntry::LogicStamp *lts = entry.GetLogicStamp();
        HashEntry::LogicStamp *lts_inMem = entry_inMem->GetLogicStamp();

        if (*lts < *lts_inMem) {
            segMgr_->ModifyDeathEntry(entry);
            __DEBUG("Ignore the UpdateIndex request, because request is expired!");
        } else {
            //this operation is need to do
            segMgr_->ModifyDeathEntry(*entry_inMem);

            uint16_t data_size = entry.GetDataSize();
            uint16_t data_inMem_size = entry_inMem->GetDataSize();

            if (data_size == 0) {
                dataTheorySize_ -= (uint64_t)(
                                              SizeOfDataHeader()
                                                      + data_inMem_size);
            } else {
                dataTheorySize_
                        += (data_size > data_inMem_size
                                                        ? ((uint64_t)(
                                                                      data_size
                                                                              - data_inMem_size))
                                                        : -((uint64_t)(
                                                                       data_inMem_size
                                                                               - data_size)));
            }
            entry_list->put(entry);

            __DEBUG("UpdateIndex request, because request is new than in memory!Now dataTheorySize_ is %ld", dataTheorySize_);
        }
        return true;

    }

    __DEBUG("Update Index: key:%s, data_len:%u, seg_id:%u, data_offset:%u",
            slice->GetKeyStr().c_str(), slice->GetDataLen(),
            slice->GetSegId(), slice->GetHashEntry().GetDataOffsetInSeg());

    return true;
}

void IndexManager::RemoveEntry(HashEntry entry) {
    Kvdb_Digest digest = entry.GetKeyDigest();

    uint32_t hash_index = KeyDigestHandle::Hash(&digest) % htSize_;
    LinkedList<HashEntry> *entry_list = hashtable_[hash_index];
    if (!entry_list) {
        return;
    }

    std::lock_guard < std::mutex > l(mtx_);
    HashEntry *entry_inMem = entry_list->getRef(entry);
    if (!entry_inMem) {
        __DEBUG("Already remove the index entry");
        return;
    }
    HashEntry::LogicStamp *lts = entry.GetLogicStamp();
    HashEntry::LogicStamp *lts_inMem = entry_inMem->GetLogicStamp();
    KVTime &t = lts->GetSegTime();
    KVTime &t_inMem = lts_inMem->GetSegTime();
    if (t_inMem == t && entry_inMem->GetDataSize() == 0) {
        entry_list->remove(entry);
        segMgr_->ModifyDeathEntry(entry);
        keyCounter_--;
        __DEBUG("Remove the index entry!");
    }

}

bool IndexManager::GetHashEntry(KVSlice *slice) {
    const Kvdb_Digest *digest = &slice->GetDigest();
    uint32_t hash_index = KeyDigestHandle::Hash(digest) % htSize_;

    LinkedList<HashEntry> *entry_list = hashtable_[hash_index];
    if (!entry_list) {
        return false;
    }

    HashEntry entry;
    entry.SetKeyDigest(*digest);

    std::lock_guard < std::mutex > l(mtx_);
    if (entry_list->search(entry)) {
        vector<HashEntry> tmp_vec = entry_list->get();
        for (vector<HashEntry>::iterator iter = tmp_vec.begin(); iter
                != tmp_vec.end(); iter++) {
            if (iter->GetKeyDigest() == *digest) {
                entry = *iter;
                slice->SetHashEntry(&entry);
                __DEBUG("IndexManger: entry : header_offset = %lu, data_offset = %u, next_header=%u",
                        entry.GetHeaderOffsetPhy(), entry.GetDataOffsetInSeg(),
                        entry.GetNextHeadOffsetInSeg());
                return true;
            }
        }
    }
    return false;
}

uint64_t IndexManager::GetDataTheorySize() const {
    std::lock_guard < std::mutex > l(mtx_);
    return dataTheorySize_;
}

uint32_t IndexManager::GetKeyCounter() const {
    std::lock_guard < std::mutex > l(mtx_);
    return keyCounter_;
}

bool IndexManager::IsSameInMem(HashEntry entry) {
    Kvdb_Digest digest = entry.GetKeyDigest();

    std::lock_guard < std::mutex > l(mtx_);

    uint32_t hash_index = KeyDigestHandle::Hash(&digest) % htSize_;
    LinkedList<HashEntry> *entry_list = hashtable_[hash_index];
    if (!entry_list) {
        __DEBUG("Not Same, because linkedlist is not created!");
        return false;
    }

    bool is_exist = entry_list->search(entry);
    if (!is_exist) {
        __DEBUG("Not Same, because entry is not exist!");
        return false;
    } else {
        HashEntry *entry_inMem = entry_list->getRef(entry);
        __DEBUG("the entry header_offset = %ld, in memory entry header_offset=%ld", entry.GetHeaderOffsetPhy(), entry_inMem->GetHeaderOffsetPhy());
        if (entry_inMem->GetHeaderOffsetPhy() == entry.GetHeaderOffsetPhy()) {
            __DEBUG("Same, because entry is same with in memory!");
            return true;
        }
    }

    __DEBUG("Not Same, because entry is not same with in memory!");
    return false;
}

uint64_t IndexManager::ComputeIndexSizeOnDevice(uint32_t ht_size) {
    uint64_t index_size = sizeof(time_t)
            + IndexManager::SizeOfHashEntryOnDisk() * ht_size;
    uint64_t index_size_pages = index_size / getpagesize();
    return (index_size_pages + 1) * getpagesize();
}

IndexManager::IndexManager(BlockDevice* bdev, SuperBlockManager* sbMgr,
                           SegmentManager* segMgr, Options &opt) :
    hashtable_(NULL), htSize_(0), keyCounter_(0), dataTheorySize_(0),
            startOff_(0), bdev_(bdev), sbMgr_(sbMgr), segMgr_(segMgr),
            options_(opt) {
    lastTime_ = new KVTime();
    return;
}

IndexManager::~IndexManager() {
    if (lastTime_) {
        delete lastTime_;
    }
    if (hashtable_) {
        destroyHashTable();
    }
}

uint32_t IndexManager::ComputeHashSizeForPower2(uint32_t number) {
    // Gets the next highest power of 2 larger than number
    number--;
    number = (number >> 1) | number;
    number = (number >> 2) | number;
    number = (number >> 4) | number;
    number = (number >> 8) | number;
    number = (number >> 16) | number;
    number++;
    return number;
}

void IndexManager::createListIfNotExist(uint32_t index) {
    if (!hashtable_[index]) {
        LinkedList<HashEntry> *entry_list = new LinkedList<HashEntry> ;
        hashtable_[index] = entry_list;
    }
}

bool IndexManager::initHashTable(uint32_t size) {
    hashtable_ = new LinkedList<HashEntry>*[htSize_];

    for (uint32_t i = 0; i < size; i++) {
        hashtable_[i] = NULL;
    }
    return true;
}

void IndexManager::destroyHashTable() {
    for (uint32_t i = 0; i < htSize_; i++) {
        if (hashtable_[i]) {
            delete hashtable_[i];
            hashtable_[i] = NULL;
        }
    }
    delete[] hashtable_;
    hashtable_ = NULL;
    return;
}

bool IndexManager::rebuildHashTable(uint64_t offset) {
    //Init hashtable
    if (!initHashTable(htSize_)) {
        return false;
    }

    __DEBUG("initHashTable success");

    //Read hashtable
    uint64_t table_length = sizeof(int) * htSize_;
    int* counter = new int[htSize_];
    if (!loadDataFromDevice((void*) counter, table_length, offset)) {
        return false;
    }
    offset += table_length;
    __DEBUG("Read hashtable success");

    //Read all hash_entry
    uint64_t length = IndexManager::SizeOfHashEntryOnDisk() * htSize_;
    HashEntryOnDisk *entry_ondisk = new HashEntryOnDisk[htSize_];
    if (!loadDataFromDevice((void*) entry_ondisk, length, offset)) {
        return false;
    }
    offset += length;
    __DEBUG("Read all hash_entry success");

    //Convert hashtable from device to memory
    if (!convertHashEntryFromDiskToMem(counter, entry_ondisk)) {
        return false;
    } __DEBUG("rebuild hash_table success");

    delete[] entry_ondisk;
    delete[] counter;

    return true;
}

bool IndexManager::loadDataFromDevice(void* data, uint64_t length,
                                      uint64_t offset) {
    if (bdev_->pRead(data, length, offset) != (ssize_t) length) {
        __ERROR("Error in reading hashtable from file\n");
        return false;
    }
    return true;
}

bool IndexManager::writeDataToDevice(void* data, uint64_t length,
                                     uint64_t offset) {
    if (bdev_->pWrite(data, length, offset) != (ssize_t) length) {
        __ERROR("Error in writing hashtable to file\n");
        return false;
    }
    return true;
}

bool IndexManager::convertHashEntryFromDiskToMem(int* counter,
                                                 HashEntryOnDisk* entry_ondisk) {
    //Convert hashtable from device to memory
    int entry_index = 0;
    for (uint32_t i = 0; i < htSize_; i++) {

        int entry_num = counter[i];
        for (int j = 0; j < entry_num; j++) {
            HashEntry entry(entry_ondisk[entry_index], *lastTime_, 0);

            createListIfNotExist(i);
            hashtable_[i]->put(entry);
            entry_index++;
        }
        if (entry_num > 0) {
            __DEBUG("read hashtable[%d]=%d", i, entry_num);
        }
    }
    keyCounter_ = entry_index;
    return true;
}

bool IndexManager::persistHashTable(uint64_t offset) {
    uint32_t entry_total = 0;

    //write hashtable to device
    uint64_t table_length = sizeof(int) * htSize_;
    int* counter = new int[htSize_];
    for (uint32_t i = 0; i < htSize_; i++) {
        counter[i] = (hashtable_[i] ? hashtable_[i]->get_size() : 0);
        entry_total += counter[i];
    }
    if (!writeDataToDevice((void*) counter, table_length, offset)) {
        return false;
    }
    offset += table_length;
    delete[] counter;
    __DEBUG("write hashtable to device success");

    //write hash entry to device
    uint64_t length = IndexManager::SizeOfHashEntryOnDisk() * entry_total;
    HashEntryOnDisk *entry_ondisk = new HashEntryOnDisk[entry_total];
    int entry_index = 0;
    for (uint32_t i = 0; i < htSize_; i++) {
        if (!hashtable_[i]) {
            continue;
        }
        vector<HashEntry> tmp_vec = hashtable_[i]->get();
        for (vector<HashEntry>::iterator iter = tmp_vec.begin(); iter
                != tmp_vec.end(); iter++) {
            entry_ondisk[entry_index++] = (iter->GetEntryOnDisk());
        }
    }

    if (!writeDataToDevice((void *) entry_ondisk, length, offset)) {
        return false;
    }
    delete[] entry_ondisk;
    __DEBUG("write all hash entry to device success");

    return true;

}

}

