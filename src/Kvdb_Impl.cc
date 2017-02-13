/* -*- Mode: C++; c-basic-offset: 3; indent-tabs-mode: nil -*- */
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <thread>
#include <map>

#include "Kvdb_Impl.h"
#include "KeyDigestHandle.h"


namespace kvdb {

    KvdbDS* KvdbDS::Create_KvdbDS(const char* filename, Options opts)
    {
        if (filename == NULL)
        {
            return NULL;
        }


        uint32_t num_entries = 0;
        uint32_t number_segments = 0;
        uint64_t db_sb_size = 0;
        uint64_t db_index_size = 0;
        uint64_t db_seg_table_size =0;
        uint64_t db_meta_size = 0;
        uint64_t db_data_region_size = 0;
        uint64_t db_size = 0;
        uint64_t device_capacity = 0;
        uint64_t data_theory_size = 0;
        
        KvdbDS* ds = new KvdbDS(filename, opts);

        uint32_t hash_table_size = ds->options_.hashtable_size;
        uint32_t segment_size = ds->options_.segment_size;

        int r = 0;
        r = ds->bdev_->Open(filename);
        if (r < 0)
        {
            delete ds;
            return NULL;
        }

        device_capacity = ds->bdev_->GetDeviceCapacity();

        //Init Superblock region
        db_sb_size = SuperBlockManager::GetSuperBlockSizeOnDevice();
        if (!ds->sbMgr_->InitSuperBlockForCreateDB(0))
        {
            delete ds;
            return NULL;
        }

        //Init Index region
        if (hash_table_size == 0)
        {
            hash_table_size = ( device_capacity / ALIGNED_SIZE ) * 2;
        }

        hash_table_size = IndexManager::ComputeHashSizeForPower2(hash_table_size);

        db_index_size = IndexManager::ComputeIndexSizeOnDevice(hash_table_size);

        if (!ds->idxMgr_->InitIndexForCreateDB(db_sb_size, hash_table_size))
        {
            delete ds;
            return NULL;
        }

        //Init Segment region
        uint64_t seg_total_size = device_capacity - db_sb_size - db_index_size;

        number_segments = SegmentManager::ComputeSegNum(seg_total_size, segment_size);
        uint64_t segtable_offset = db_sb_size + db_index_size;

        if (!ds->segMgr_->InitSegmentForCreateDB(segtable_offset,
                                                segment_size,
                                                number_segments))
        {
            delete ds;
            return NULL;
        }


        db_seg_table_size = SegmentManager::ComputeSegTableSizeOnDisk(number_segments);

        db_meta_size  = db_sb_size + db_index_size + db_seg_table_size;
        db_data_region_size = ds->segMgr_->GetDataRegionSize();
        db_size = db_meta_size + db_data_region_size;

        r = ds->bdev_->SetNewDBZero(db_meta_size);
        if (r < 0)
        {
            delete ds;
            return NULL;
        }


        DBSuperBlock sb(MAGIC_NUMBER, hash_table_size, num_entries,
                        segment_size, number_segments,
                        0, db_sb_size, db_index_size, db_seg_table_size,
                        db_data_region_size, device_capacity, data_theory_size);
        ds->sbMgr_->SetSuperBlock(sb);

        __INFO("\nCreateKvdbDS table information:\n"
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

        ds->seg_ = new SegmentSlice(ds->segMgr_,ds->idxMgr_, ds->bdev_, ds->options_.expired_time);
        ds->startThds();

        return ds; 

    }


    bool KvdbDS::writeMetaDataToDevice()
    {
        if (!idxMgr_->WriteIndexToDevice())
        {
            return false;
        }

        if (!segMgr_->WriteSegmentTableToDevice())
        {
            return false;
        }

        if (!sbMgr_->WriteSuperBlockToDevice())
        {
            return false;
        }

        return true;
    }

    bool KvdbDS::readMetaDataFromDevice()
    {
        uint64_t offset = 0;
        if (!sbMgr_->LoadSuperBlockFromDevice(offset))
        {
            return false;
        }

        uint32_t hashtable_size = sbMgr_->GetHTSize();
        uint64_t db_sb_size = SuperBlockManager::GetSuperBlockSizeOnDevice();
        offset += db_sb_size;
        if (!idxMgr_->LoadIndexFromDevice(offset, hashtable_size))
        {
            return false;
        }

        uint64_t db_index_size = IndexManager::ComputeIndexSizeOnDevice(hashtable_size);
        offset += db_index_size;
        uint32_t segment_size = sbMgr_->GetSegmentSize();
        uint32_t number_segments = sbMgr_->GetSegmentNum();
        uint32_t current_seg =sbMgr_->GetCurSegmentId();
        if (!segMgr_->LoadSegmentTableFromDevice(offset, segment_size, number_segments, current_seg))
        {
            return false;
        }

        seg_ = new SegmentSlice(segMgr_, idxMgr_,  bdev_, options_.expired_time);

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


    KvdbDS* KvdbDS::Open_KvdbDS(const char* filename, Options opts)
    {
        KvdbDS *instance_ = new KvdbDS(filename, opts);

        if (!instance_->openDB())
        {
            delete instance_;
            return NULL;
        }
        return instance_;

    }

    bool KvdbDS::openDB()
    {
        if (bdev_->Open(fileName_) < 0)
        {
            __ERROR("Could not open device\n");
            return false;
        }
        if (!readMetaDataFromDevice())
        {
            __ERROR("Could not read  hash table file\n");
            return false;
        }
        startThds();
        return true;
    }

    bool KvdbDS::closeDB()
    {
        if (!writeMetaDataToDevice())
        {
            __ERROR("Could not to write metadata to device\n");
            return false;
        }
        stopThds();
        return true;
    }

    void KvdbDS::startThds()
    {
        reqMergeT_stop_.store(false);
        reqMergeT_ = std::thread(&KvdbDS::ReqMergeThdEntry,this);

        segWriteT_stop_.store(false);
        for(int i = 0; i < options_.seg_write_thread; i++)
        {
            segWriteTP_.push_back(std::thread(&KvdbDS::SegWriteThdEntry, this));
        }

        segTimeoutT_stop_.store(false);
        segTimeoutT_ = std::thread(&KvdbDS::SegTimeoutThdEntry, this);

        segReaperT_stop_.store(false);
        segReaperT_ = std::thread(&KvdbDS::SegReaperThdEntry, this);

        gcT_stop_.store(false);
        gcT_ = std::thread(&KvdbDS::GCThdEntry, this);
    }

    void KvdbDS::stopThds()
    {
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

    KvdbDS::~KvdbDS()
    {
        closeDB();
        delete gcMgr_;
        delete idxMgr_;
        delete segMgr_;
        delete sbMgr_;
        delete bdev_;
        delete seg_;

    }

    KvdbDS::KvdbDS(const string& filename, Options opts) :
        fileName_(filename),
        seg_(NULL),
        options_(opts),
        reqMergeT_stop_(false),
        segWriteT_stop_(false),
        segTimeoutT_stop_(false),
        segReaperT_stop_(false),
        gcT_stop_(false)
    {
        bdev_ = BlockDevice::CreateDevice();
        sbMgr_ = new SuperBlockManager(bdev_, options_);
        segMgr_ = new SegmentManager(bdev_, sbMgr_, options_);
        idxMgr_ = new IndexManager(bdev_, sbMgr_, segMgr_, options_);
        gcMgr_ = new GcManager(bdev_, idxMgr_, segMgr_, options_);
    }


    bool KvdbDS::Insert(const char* key, uint32_t key_len, const char* data, uint16_t length)
    {
        bool res = false;
        if (key == NULL)
        {
            return res;
        }

        if (length > segMgr_->GetMaxValueLength())
        {
            return res;
        }

        KVSlice slice(key, key_len, data, length);

        res = insertKey(slice);

        return res;

    }

    bool KvdbDS::Delete(const char* key, uint32_t key_len)
    {
        return Insert(key, key_len, NULL, 0);
    }

    bool KvdbDS::Get(const char* key, uint32_t key_len, string &data) 
    {
        bool res = false;
        if (key == NULL)
        {
            return res;
        }

        KVSlice slice(key, key_len, NULL, 0);

        res = idxMgr_->GetHashEntry(&slice);
        if (!res)
        {
            //The key is not exist
            return res;
        }

        res = readData(slice, data);
        return res;

    }

    bool KvdbDS::insertKey(KVSlice& slice)
    {
        Request *req = new Request(slice);
        reqQue_.Enqueue_Notify(req);
        req->Wait();
        bool res = updateMeta(req);
        delete req;
        return res;
    }

    bool KvdbDS::updateMeta(Request *req)
    {
        bool res = req->GetWriteStat();
        // update index
        if ( res )
        {
            KVSlice *slice = &req->GetSlice();
            res = idxMgr_->UpdateIndex(slice);
        }
        // minus the segment delete counter
        SegmentSlice *seg = req->GetSeg();
        if (!seg->CommitedAndGetNum())
        {
            segReaperQue_.Enqueue_Notify(seg);
        }
        return res;
    }


    bool KvdbDS::readData(KVSlice &slice, string &data)
    {
        HashEntry *entry;
        entry = &slice.GetHashEntry();

        uint64_t data_offset = 0;
        if (!segMgr_->ComputeDataOffsetPhyFromEntry(entry, data_offset))
        {
            return false;
        }

        uint16_t data_len = entry->GetDataSize();
        char *mdata = new char[data_len];
        if (bdev_->pRead(mdata, data_len, data_offset) != (ssize_t)data_len)
        {
            __ERROR("Could not read data at position");
            delete[] mdata;
            return false;
        }
        data.assign(mdata, data_len);
        delete[] mdata;

        __DEBUG("get key: %s, data offset %ld, head_offset is %ld", slice.GetKeyStr().c_str(), data_offset, entry->GetHeaderOffsetPhy());

        return true;
    }


    void KvdbDS::ReqMergeThdEntry()
    {
        __DEBUG("Requests Merge thread start!!");
        std::unique_lock<std::mutex> lck_seg(segMtx_, std::defer_lock);
        while(!reqMergeT_stop_.load())
        {
            Request  *req = reqQue_.Wait_Dequeue();
            if ( req )
            {
                lck_seg.lock();
                if(seg_->TryPut(req))
                {
                    seg_->Put(req);
                }
                else
                {
                     seg_->Complete();
                     segWriteQue_.Enqueue_Notify(seg_);
                     seg_ = new SegmentSlice(segMgr_, idxMgr_, bdev_, options_.expired_time);
                     seg_->Put(req);
                }
                lck_seg.unlock();
            }

        }
        __DEBUG("Requests Merge thread stop!!");
    }

    void KvdbDS::SegWriteThdEntry()
    {
        __DEBUG("Segment write thread start!!");
        while (!segWriteT_stop_)
        {
            SegmentSlice *seg = segWriteQue_.Wait_Dequeue();
            if ( seg )
            {
                uint32_t seg_id = 0;
                bool res;
                while ( !segMgr_->Alloc(seg_id) )
                {
                    res = gcMgr_->ForeGC();
                    if (!res)
                    {
                        __ERROR("Cann't get a new Empty Segment.\n");
                        seg->Notify(res);
                    }
                }

                uint32_t free_size = seg->GetFreeSize();
                res = seg->WriteSegToDevice(seg_id);
                if ( res )
                {
                    segMgr_->Use(seg_id, free_size);
                }
                else
                {
                    segMgr_->FreeForFailed(seg_id);
                }
                seg->Notify(res);

                __DEBUG("Segment thread write seg to device, seg_id:%d %s",
                        seg_id, res==true? "Success":"Failed");

            }
        }
        __DEBUG("Segment write thread stop!!");
    }


    void KvdbDS::SegTimeoutThdEntry()
    {
        __DEBUG("Segment Timeout thread start!!");
        std::unique_lock<std::mutex> lck(segMtx_, std::defer_lock);
        while (!segTimeoutT_stop_)
        {
            lck.lock();
            if (seg_->IsExpired())
            {
                seg_->Complete();

                segWriteQue_.Enqueue_Notify(seg_);
                seg_ = new SegmentSlice(segMgr_, idxMgr_, bdev_, options_.expired_time);
            }
            lck.unlock();

            usleep(options_.expired_time);
        }
        __DEBUG("Segment Timeout thread stop!!");
    }

    void KvdbDS::SegReaperThdEntry()
    {
        __DEBUG("Segment reaper thread start!!");

        while (!segReaperT_stop_)
        {
            SegmentSlice *seg = segReaperQue_.Wait_Dequeue();
            if ( seg )
            {
                seg->CleanDeletedEntry();
                __DEBUG("Segment reaper delete seg_id = %d", seg->GetSegId());
                delete seg;
            }
        }
        __DEBUG("Segment write thread stop!!");
    }

    void KvdbDS::Do_GC()
    {
        __INFO("Application call GC!!!!!");
        return gcMgr_->FullGC();
    }

    void KvdbDS::GCThdEntry()
    {
        __DEBUG("GC thread start!!");
        while (!gcT_stop_)
        {
            gcMgr_->BackGC();
            usleep(1000000);
        }
        __DEBUG("GC thread stop!!");
    }
}

