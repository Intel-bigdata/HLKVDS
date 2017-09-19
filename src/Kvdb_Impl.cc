#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <thread>
#include <map>

#include "Kvdb_Impl.h"
#include "KeyDigestHandle.h"
#include "KvdbIter.h"

namespace hlkvds {

KVDS* KVDS::Create_KVDS(const char* filename, Options opts) {
    if (filename == NULL) {
        return NULL;
    }

    uint32_t num_entries = 0;
    uint32_t number_segments = 0;
    uint64_t db_sb_size = 0;
    uint64_t db_index_size = 0;
    uint64_t db_seg_table_size = 0;
    uint64_t db_meta_size = 0;
    uint64_t db_data_region_size = 0;
    uint64_t db_size = 0;
    uint64_t device_capacity = 0;
    uint64_t data_theory_size = 0;

    KVDS* ds = new KVDS(filename, opts);

    uint32_t hash_table_size = ds->options_.hashtable_size;
    uint32_t segment_size = ds->options_.segment_size;

    int r = 0;
    r = ds->bdev_->Open(filename);
    if (r < 0) {
        delete ds;
        return NULL;
    } __DEBUG("Open device success.");

    device_capacity = ds->bdev_->GetDeviceCapacity();
    //device capacity should be larger than size of (sb+index)
    if (device_capacity < 8192) {
        __ERROR("The capacity of device is too small.");
        delete ds;
        return NULL;
    }

    //Init Superblock region
    db_sb_size = SuperBlockManager::GetSuperBlockSizeOnDevice();
    __DEBUG("super block size; %ld",db_sb_size);
    if (!ds->sbMgr_->InitSuperBlockForCreateDB(0)) {
        delete ds;
        return NULL;
    }

    __DEBUG("Init super block region success.");
    //Init Index region
    if (hash_table_size == 0) {
        hash_table_size = (device_capacity / ALIGNED_SIZE) * 2;
    }

    hash_table_size = IndexManager::ComputeHashSizeForPower2(hash_table_size);

    db_index_size = IndexManager::ComputeIndexSizeOnDevice(hash_table_size);
    __DEBUG("index region size; %ld",db_index_size);

    if (!ds->idxMgr_->InitIndexForCreateDB(db_sb_size, hash_table_size)) {
        delete ds;
        return NULL;
    } __DEBUG("Init index region success.");

    //Init Segment region
    uint64_t seg_total_size = device_capacity - db_sb_size - db_index_size;

    if (segment_size <= 0 || segment_size > seg_total_size) {
        __ERROR("Improper segment size, %d",segment_size);
        //delete ds;
        return NULL;
    }

    number_segments = SegmentManager::ComputeSegNum(seg_total_size,
                                                    segment_size);

    uint64_t segtable_offset = db_sb_size + db_index_size;

    if (!ds->segMgr_->InitSegmentForCreateDB(segtable_offset, segment_size,
                                             number_segments)) {
        __ERROR("Segment region init failed.");
        delete ds;
        return NULL;
    }

    __DEBUG("Init segment region success.");
    db_seg_table_size
            = SegmentManager::ComputeSegTableSizeOnDisk(number_segments);

    db_meta_size = db_sb_size + db_index_size + db_seg_table_size;
    db_data_region_size = ds->segMgr_->GetDataRegionSize();
    db_size = db_meta_size + db_data_region_size;

    r = ds->bdev_->SetNewDBZero(db_meta_size);
    if (r < 0) {
        delete ds;
        return NULL;
    }

    DBSuperBlock sb(MAGIC_NUMBER, hash_table_size, num_entries, segment_size,
                    number_segments, 0, db_sb_size, db_index_size,
                    db_seg_table_size, db_data_region_size, device_capacity,
                    data_theory_size);
    ds->sbMgr_->SetSuperBlock(sb);

    __INFO("\nCreateKVDS table information:\n"
            "\t hashtable_size            : %d\n"
            "\t num_entries               : %d\n"
            "\t segment_size              : %d Bytes\n"
            "\t number_segments           : %d\n"
            "\t Database Superblock Size  : %ld Bytes\n"
            "\t Database Index Size       : %ld Bytes\n"
            "\t Database Seg Table Size   : %ld Bytes\n"
            "\t Total DB Meta Region Size : %ld Bytes\n"
            "\t Total DB Data Region Size : %ld Bytes\n"
            "\t Total DB Total Size       : %ld Bytes\n"
            "\t Total Device Size         : %ld Bytes",
            hash_table_size, num_entries,
            segment_size, number_segments, db_sb_size,
            db_index_size, db_seg_table_size, db_meta_size,
            db_data_region_size, db_size, device_capacity);

    ds->seg_ = new SegForReq(ds->segMgr_, ds->idxMgr_, ds->bdev_,
                                ds->options_.expired_time);
    ds->startThds();

    return ds;

}

Iterator* KVDS::NewIterator() {
#ifdef WITH_ITERATOR
    return new KvdbIter(idxMgr_, segMgr_, bdev_);
#else
    return NULL;
#endif
}

void KVDS::printDbStates() {

    uint32_t hash_table_size = sbMgr_->GetHTSize();
    uint32_t num_entries = idxMgr_->GetKeyCounter();
    uint32_t segment_size = sbMgr_->GetSegmentSize();
    uint32_t number_segments = sbMgr_->GetSegmentNum();
    uint32_t free_segment = segMgr_->GetTotalFreeSegs();
    uint64_t db_sb_size = sbMgr_->GetSbSize();
    uint64_t db_index_size = sbMgr_->GetIndexSize();
    uint64_t db_seg_table_size = sbMgr_->GetSegTableSize();
    uint64_t db_meta_size = db_sb_size + db_index_size + db_seg_table_size;
    uint64_t db_data_region_size = sbMgr_->GetDataRegionSize();
    uint64_t db_size = db_meta_size + db_data_region_size;
    uint64_t device_capacity = sbMgr_->GetDeviceCapacity();

    __INFO("\nDB state information:\n"
            "\t HashTable size            : %d\n"
            "\t # of entries              : %d\n"
            "\t Segment size              : %d Bytes\n"
            "\t # of total segments       : %d\n"
            "\t # of free segments        : %d\n"
            "\t Database Superblock Size  : %ld Bytes\n"
            "\t Database Index Size       : %ld Bytes\n"
            "\t Database Seg Table Size   : %ld Bytes\n"
            "\t Total DB Meta Region Size : %ld Bytes\n"
            "\t Total DB Data Region Size : %ld Bytes\n"
            "\t Total DB Total Size       : %ld Bytes\n"
            "\t Total Device Size         : %ld Bytes\n"
            "\t Request Queue Size        : %d\n"
            "\t Segment Reaper Queue Size : %d\n"
            "\t Segment Write Queue Size  : %d",
            hash_table_size, num_entries,
            segment_size, number_segments,free_segment, db_sb_size,
            db_index_size, db_seg_table_size, db_meta_size,
            db_data_region_size, db_size, device_capacity,getReqQueSize(),getSegReaperQueSize(),getSegWriteQueSize());
}

bool KVDS::writeMetaDataToDevice() {
    if (!idxMgr_->WriteIndexToDevice()) {
        return false;
    }

    if (!segMgr_->WriteSegmentTableToDevice()) {
        return false;
    }

    if (!sbMgr_->WriteSuperBlockToDevice()) {
        return false;
    }

    return true;
}

bool KVDS::readMetaDataFromDevice() {
    uint64_t offset = 0;
    if (!sbMgr_->LoadSuperBlockFromDevice(offset)) {
        return false;
    }

    uint32_t hashtable_size = sbMgr_->GetHTSize();
    uint64_t db_sb_size = SuperBlockManager::GetSuperBlockSizeOnDevice();
    offset += db_sb_size;
    if (!idxMgr_->LoadIndexFromDevice(offset, hashtable_size)) {
        return false;
    }

    uint64_t db_index_size =
            IndexManager::ComputeIndexSizeOnDevice(hashtable_size);
    offset += db_index_size;
    uint32_t segment_size = sbMgr_->GetSegmentSize();
    uint32_t number_segments = sbMgr_->GetSegmentNum();
    uint32_t current_seg = sbMgr_->GetCurSegmentId();
    if (!segMgr_->LoadSegmentTableFromDevice(offset, segment_size,
                                             number_segments, current_seg)) {
        return false;
    }

    seg_ = new SegForReq(segMgr_, idxMgr_, bdev_, options_.expired_time);

    __INFO("\nReading meta information from file:\n"
            "\t hashtable_size            : %d\n"
            "\t num_entries               : %d\n"
            "\t segment_size              : %d Bytes\n"
            "\t number_segments           : %d\n"
            "\t Database Superblock Size  : %ld Bytes\n"
            "\t Database Index Size       : %ld Bytes\n"
            "\t Database Seg Table Size   : %ld Bytes\n"
            "\t Total DB Meta Region Size : %ld Bytes\n"
            "\t Total DB Data Region Size : %ld Bytes\n"
            "\t Total DB Total Size       : %ld Bytes\n"
            "\t Total Device Size         : %ld Bytes\n"
            "\t Current Segment ID        : %d\n"
            "\t DB Data Theory Size       : %ld Bytes",
            sbMgr_->GetHTSize(), sbMgr_->GetElementNum(),
            sbMgr_->GetSegmentSize(),
            sbMgr_->GetSegmentNum(), sbMgr_->GetSbSize(),
            sbMgr_->GetIndexSize(), sbMgr_->GetSegTableSize(),
            (sbMgr_->GetSbSize() + sbMgr_->GetIndexSize() + sbMgr_->GetSegTableSize()),
            sbMgr_->GetDataRegionSize(),
            (sbMgr_->GetSbSize() + sbMgr_->GetIndexSize() + sbMgr_->GetSegTableSize() + sbMgr_->GetDataRegionSize()),
            sbMgr_->GetDeviceCapacity(), sbMgr_->GetCurSegmentId(), sbMgr_->GetDataTheorySize());

    return true;
}

KVDS* KVDS::Open_KVDS(const char* filename, Options opts) {
    KVDS *instance_ = new KVDS(filename, opts);

    Status s = instance_->openDB();
    if (!s.ok()) {
        delete instance_;
        return NULL;
    }
    return instance_;

}

Status KVDS::openDB() {
    if (bdev_->Open(fileName_) < 0) {
        __ERROR("Could not open device\n");
        return Status::IOError("Could not open device");
    }
    if (!readMetaDataFromDevice()) {
        __ERROR("Could not read  hash table file\n");
        return Status::IOError("Could not read  hash table file");
    }
    startThds();
    return Status::OK();
}

Status KVDS::closeDB() {
    if (!writeMetaDataToDevice()) {
        __ERROR("Could not to write metadata to device\n");
        return Status::IOError("Could not to write metadata to device");
    }
    stopThds();
    return Status::OK();
}

void KVDS::startThds() {
    reqMergeT_stop_.store(false);
    reqMergeT_ = std::thread(&KVDS::ReqMergeThdEntry, this);

    segWriteT_stop_.store(false);
    for (int i = 0; i < options_.seg_write_thread; i++) {
        segWriteTP_.push_back(std::thread(&KVDS::SegWriteThdEntry, this));
    }

    segTimeoutT_stop_.store(false);
    segTimeoutT_ = std::thread(&KVDS::SegTimeoutThdEntry, this);

    segReaperT_stop_.store(false);
    segReaperT_ = std::thread(&KVDS::SegReaperThdEntry, this);

    gcT_stop_.store(false);
    gcT_ = std::thread(&KVDS::GCThdEntry, this);
}

void KVDS::stopThds() {
    gcT_stop_.store(true);
    gcT_.join();

    segReaperT_stop_.store(true);
    segReaperT_.join();

    segTimeoutT_stop_.store(true);
    segTimeoutT_.join();

    segWriteT_stop_.store(true);
    for(auto &th : segWriteTP_)
    {
        th.join();
    }

    reqMergeT_stop_.store(true);
    reqMergeT_.join();
}

KVDS::~KVDS() {
    closeDB();
    delete gcMgr_;
    delete idxMgr_;
    delete segMgr_;
    delete sbMgr_;
    delete bdev_;
    delete seg_;
    if(!options_.disable_cache){
        delete rdCache_;
    }

}

KVDS::KVDS(const string& filename, Options opts) :
    fileName_(filename), seg_(NULL), options_(opts), reqMergeT_stop_(false),
            segWriteT_stop_(false), segTimeoutT_stop_(false),
            segReaperT_stop_(false), gcT_stop_(false) {
    bdev_ = BlockDevice::CreateDevice();
    sbMgr_ = new SuperBlockManager(bdev_, options_);
    segMgr_ = new SegmentManager(bdev_, sbMgr_, options_);
    idxMgr_ = new IndexManager(bdev_, sbMgr_, segMgr_, options_);
    gcMgr_ = new GcManager(bdev_, idxMgr_, segMgr_, options_);
    if(!options_.disable_cache){
        rdCache_ = new dslab::ReadCache(dslab::CachePolicy(options_.cache_policy), (size_t) options_.cache_size, options_.slru_partition);
    }
}

Status KVDS::Insert(const char* key, uint32_t key_len, const char* data,
                      uint16_t length) {
    if (key == NULL || key[0] == '\0') {
        return Status::InvalidArgument("Key is null or empty.");
    }

    //__INFO("!!!%d", segMgr_->GetMaxValueLength());

    if (length > segMgr_->GetMaxValueLength()) {
        return Status::NotSupported(
                                    "Data length cann't be longer than max segment size");
    }

    KVSlice slice(key, key_len, data, length);

    if(!options_.disable_cache && !slice.GetDataLen()){
        rdCache_->Put(slice.GetKeyStr(),slice.GetDataStr());
    }

    return insertKey(slice);

}

Status KVDS::Delete(const char* key, uint32_t key_len) {
    return Insert(key, key_len, NULL, 0);
}

Status KVDS::Get(const char* key, uint32_t key_len, string &data) {
    bool res = false;
    if (key == NULL) {
        return Status::InvalidArgument("Key is null.");
    }

    KVSlice slice(key, key_len, NULL, 0);

    res = idxMgr_->GetHashEntry(&slice);
    if (!res) {
        //The key is not exist
        return Status::NotFound("Key is not found.");
    }
    if(!options_.disable_cache){
	if(rdCache_->Get(slice.GetKeyStr(), data)){
            return  Status::OK();
        }else{
	    Status _status = readData(slice, data);
            if(_status.ok()){
		rdCache_->Put(slice.GetKeyStr(), data);
	    }
	    return _status;
        }
    }else{
        Status _status = readData(slice, data);
        cout<<_status.ToString()<<endl;
        return _status;
    }
}

Status KVDS::InsertBatch(WriteBatch *batch)
{
    if (batch->batch_.empty()) {
        return Status::OK();
    }

    uint32_t seg_id = 0;
    bool ret;
    //TODO: use GcSegment first, need abstract segment.
    while (!segMgr_->Alloc(seg_id)) {
        ret = gcMgr_->ForeGC();
        if (!ret) {
            __ERROR("Cann't get a new Empty Segment.\n");
            return Status::Aborted("Can't allocate a Empty Segment.");
        }
    }

    SegForSlice *seg = new SegForSlice(segMgr_, idxMgr_, bdev_);
    for (std::list<KVSlice *>::iterator iter = batch->batch_.begin();
            iter != batch->batch_.end(); iter++) {
        if (seg->TryPut(*iter)) {
            seg->Put(*iter);
        }
        else {
            __ERROR("The Batch is too large, can't put in one segment");
            delete seg;
            return Status::Aborted("Batch is too large.");
        }
    }
    if(!options_.disable_cache){
        for (std::list<KVSlice *>::iterator iter = batch->batch_.begin();
            iter != batch->batch_.end(); iter++) {
	    if(!(*iter)->GetDataLen()){//no "" should be put in to the cache
	    	rdCache_->Put((*iter)->GetKeyStr(), (*iter)->GetDataStr());
    	    }
        }
    }
    seg->SetSegId(seg_id);
    ret = seg->WriteSegToDevice();
    if(!ret) {
        __ERROR("Write batch segment to device failed");
        segMgr_->FreeForFailed(seg_id);
        delete seg;
        return Status::IOError("could not write batch segment to device ");
    }

    uint32_t free_size = seg->GetFreeSize();
    segMgr_->Use(seg_id, free_size);
    seg->UpdateToIndex();
    delete seg;
    return Status::OK();
}

Status KVDS::insertKey(KVSlice& slice) {
    Request *req = new Request(slice);
    reqQue_.Enqueue_Notify(req);
    req->Wait();
    Status s = updateMeta(req);
    delete req;
    return s;
}

Status KVDS::updateMeta(Request *req) {
    bool res = req->GetWriteStat();
    // update index
    if (!res) {
        return Status::Aborted("Write failed");
    }
    KVSlice *slice = &req->GetSlice();
    res = idxMgr_->UpdateIndex(slice);
    // minus the segment delete counter
    SegForReq *seg = req->GetSeg();
    if (!seg->CommitedAndGetNum()) {
        segReaperQue_.Enqueue_Notify(seg);
    }
    return Status::OK();
}

Status KVDS::readData(KVSlice &slice, string &data) {
    HashEntry *entry;
    entry = &slice.GetHashEntry();

    uint64_t data_offset = 0;
    if (!segMgr_->ComputeDataOffsetPhyFromEntry(entry, data_offset)) {
        return Status::Aborted("Compute data offset failed.");
    }

    uint16_t data_len = entry->GetDataSize();

    if (data_len == 0) {
        //The key is not exist
        return Status::NotFound("Key is not found.");
    }

    char *mdata = new char[data_len];
    if (bdev_->pRead(mdata, data_len, data_offset) != (ssize_t) data_len) {
        __ERROR("Could not read data at position");
        delete[] mdata;
        return Status::IOError("Could not read data at position.");
    }
    data.assign(mdata, data_len);
    delete[] mdata;

#ifdef WITH_ITERATOR
    ////TODO: test read key, need remove to other place
    //uint64_t key_offset = 0;
    //uint16_t key_len = entry->GetKeySize();
    //segMgr_->ComputeSegOffsetFromOffset(data_offset, key_offset);
    //key_offset += entry->GetNextHeadOffsetInSeg() - key_len;
    //char *mkey = new char[key_len];
    //if (bdev_->pRead(mkey, key_len, key_offset) != (ssize_t) key_len) {
    //    __ERROR("Could not read key at position");
    //    delete[] mkey;
    //    return Status::IOError("Could not read key at position.");
    //}
    //__INFO("!!!!!!!!!!!!!!!!!! Key is %s!!!!", mkey);
    //delete[] mkey;
    ////test read key END!
#endif

    __DEBUG("get key: %s, data offset %ld, head_offset is %ld", slice.GetKeyStr().c_str(), data_offset, entry->GetHeaderOffsetPhy());

    return Status::OK();
}

void KVDS::ReqMergeThdEntry() {
    __DEBUG("Requests Merge thread start!!");
    std::unique_lock < std::mutex > lck_seg(segMtx_, std::defer_lock);
    while (!reqMergeT_stop_.load()) {
        Request *req = reqQue_.Wait_Dequeue();
        if (req) {
            lck_seg.lock();
            if (seg_->TryPut(req)) {
                seg_->Put(req);
            } else {
                seg_->Complete();
                segWriteQue_.Enqueue_Notify(seg_);
                seg_ = new SegForReq(segMgr_, idxMgr_, bdev_,
                                    options_.expired_time);
                seg_->Put(req);
            }
            lck_seg.unlock();
        }

    } __DEBUG("Requests Merge thread stop!!");
}

void KVDS::SegWriteThdEntry() {
    __DEBUG("Segment write thread start!!");
    while (!segWriteT_stop_) {
        SegForReq *seg = segWriteQue_.Wait_Dequeue();
        if (seg) {
            uint32_t seg_id = 0;
            bool res;
            while (!segMgr_->Alloc(seg_id)) {
                res = gcMgr_->ForeGC();
                if (!res) {
                    __ERROR("Cann't get a new Empty Segment.\n");
                    seg->Notify(res);
                }
            }

            uint32_t free_size = seg->GetFreeSize();
            seg->SetSegId(seg_id);
            res = seg->WriteSegToDevice();
            if (res) {
                segMgr_->Use(seg_id, free_size);
            } else {
                segMgr_->FreeForFailed(seg_id);
            }
            seg->Notify(res);

            __DEBUG("Segment thread write seg to device, seg_id:%d %s",
                    seg_id, res==true? "Success":"Failed");

        }
    } __DEBUG("Segment write thread stop!!");
}

void KVDS::SegTimeoutThdEntry() {
    __DEBUG("Segment Timeout thread start!!");
    std::unique_lock < std::mutex > lck(segMtx_, std::defer_lock);
    while (!segTimeoutT_stop_) {
        lck.lock();
        if (seg_->IsExpired()) {
            seg_->Complete();

            segWriteQue_.Enqueue_Notify(seg_);
            seg_ = new SegForReq(segMgr_, idxMgr_, bdev_,
                                options_.expired_time);
        }
        lck.unlock();

        usleep(options_.expired_time);
    } __DEBUG("Segment Timeout thread stop!!");
}

void KVDS::SegReaperThdEntry() {
    __DEBUG("Segment reaper thread start!!");

    while (!segReaperT_stop_) {
        SegForReq *seg = segReaperQue_.Wait_Dequeue();
        if (seg) {
            seg->CleanDeletedEntry();
            __DEBUG("Segment reaper delete seg_id = %d", seg->GetSegId());
            delete seg;
        }
    } __DEBUG("Segment write thread stop!!");

    //Clean the Queue
    while (!segReaperQue_.empty()) {
        SegForReq *seg = segReaperQue_.Wait_Dequeue();
        if (seg) {
            seg->CleanDeletedEntry();
            __DEBUG("Segment reaper delete seg_id = %d", seg->GetSegId());
            delete seg;
        }
    }
}

void KVDS::Do_GC() {
    __INFO("Application call GC!!!!!");
    return gcMgr_->FullGC();
}

void KVDS::GCThdEntry() {
    __DEBUG("GC thread start!!");
    while (!gcT_stop_) {
        gcMgr_->BackGC();
        usleep(1000000);
    } __DEBUG("GC thread stop!!");
}
}

