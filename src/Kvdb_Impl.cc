#include <stdlib.h>
#include "Kvdb_Impl.h"
#include "KvdbIter.h"
#include "Db_Structure.h"

#include "BlockDevice.h"
#include "SuperBlockManager.h"
#include "IndexManager.h"
#include "MetaStor.h"
#include "DataStor.h"

using namespace std;
using namespace dslab;

namespace hlkvds {

KVDS* KVDS::Create_KVDS(const char* filename, Options opts) {
    if (filename == NULL) {
        return NULL;
    }

    KVDS* ds = new KVDS(filename, opts);

    if (!ds->metaStor_->CreateMetaData()) {
        delete ds;
        return NULL;
    }

    __INFO("\nCreateKVDS Success!!!\n");
    ds->printDbStates();

    ds->dataStor_->InitSegment();
    ds->idxMgr_->InitDataStor(ds->dataStor_);
    ds->startThds();

    return ds;

}

Iterator* KVDS::NewIterator() {
    return new KvdbIter(idxMgr_, dataStor_, bdev_);
}

void KVDS::printDbStates() {

    uint32_t hash_table_size = sbMgr_->GetHTSize();
    uint32_t num_entries = idxMgr_->GetKeyCounter();
    uint32_t segment_size = sbMgr_->GetSegmentSize();
    uint32_t number_segments = sbMgr_->GetSegmentNum();
    uint32_t free_segment = dataStor_->GetTotalFreeSegs();
    uint64_t db_sb_size = sbMgr_->GetSbSize();
    uint64_t db_index_size = sbMgr_->GetIndexSize();
    uint64_t db_seg_table_size = sbMgr_->GetSegTableSize();
    uint64_t db_meta_size = db_sb_size + db_index_size + db_seg_table_size;
    uint64_t db_data_region_size = sbMgr_->GetDataRegionSize();
    uint64_t db_size = db_meta_size + db_data_region_size;
    uint64_t device_capacity = sbMgr_->GetDeviceCapacity();

    __INFO("\n DB Static information:\n"
            "\t hashtable_size            : %d\n"
            "\t segment_size              : %d Bytes\n"
            "\t number_segments           : %d\n"
            "\t Database Superblock Size  : %ld Bytes\n"
            "\t Database Index Size       : %ld Bytes\n"
            "\t Database Seg Table Size   : %ld Bytes\n"
            "\t Total DB Meta Region Size : %ld Bytes\n"
            "\t Total DB Data Region Size : %ld Bytes\n"
            "\t Total DB Total Size       : %ld Bytes\n"
            "\t Total Device Size         : %ld Bytes",
            hash_table_size, segment_size,
            number_segments, db_sb_size,
            db_index_size, db_seg_table_size, db_meta_size,
            db_data_region_size, db_size, device_capacity);

    __INFO("\n DB Dynamic information: \n"
            "\t # of entries              : %d\n"
            "\t # of free segments        : %d\n"
            "\t Current Segment ID        : %d\n"
            "\t DB Data Theory Size       : %ld Bytes\n"
            "\t Request Queue Size        : %d\n"
            "\t Segment Write Queue Size  : %d\n"
            "\t Segment Reaper Queue Size : %d",
            num_entries, free_segment,
            sbMgr_->GetCurSegmentId(),
            sbMgr_->GetDataTheorySize(), dataStor_->GetReqQueSize(),
            dataStor_->GetSegWriteQueSize(), idxMgr_->GetSegReaperQueSize());
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
    if (!metaStor_->LoadMetaData()) {
        return Status::IOError("Could not read meta data");
    }

    printDbStates();

    dataStor_->InitSegment();
    idxMgr_->InitDataStor(dataStor_);
    startThds();
    return Status::OK();
}

Status KVDS::closeDB() {
    if (!metaStor_->PersistMetaData()) {
        __ERROR("Could not to write metadata to device\n");
        return Status::IOError("Could not to write metadata to device");
    }
    stopThds();
    return Status::OK();
}

void KVDS::startThds() {
    idxMgr_->StartThds();
    dataStor_->StartThds();
}

void KVDS::stopThds() {
    dataStor_->StopThds();
    idxMgr_->StopThds();
}

KVDS::~KVDS() {
    closeDB();
    delete idxMgr_;
    delete sbMgr_;
    delete bdev_;
    if(!options_.disable_cache){
        delete rdCache_;
    }
    delete metaStor_;
    delete dataStor_;

}

KVDS::KVDS(const char* filename, Options opts) :
    sbMgr_(NULL), idxMgr_(NULL), bdev_(NULL), rdCache_(NULL), metaStor_(NULL), dataStor_(NULL), options_(opts) {

    bdev_ = BlockDevice::CreateDevice();
    sbMgr_ = new SuperBlockManager(options_);
    idxMgr_ = new IndexManager(sbMgr_, options_);

    if(!options_.disable_cache){
        rdCache_ = new ReadCache(CachePolicy(options_.cache_policy), (size_t) options_.cache_size, options_.slru_partition);
    }

    dataStor_ = new SimpleDS_Impl(options_, bdev_, sbMgr_, idxMgr_);
    metaStor_ = new MetaStor(filename, bdev_, sbMgr_, idxMgr_, dataStor_, options_);
}

Status KVDS::Insert(const char* key, uint32_t key_len, const char* data,
                      uint16_t length) {
    if (key == NULL || key[0] == '\0') {
        return Status::InvalidArgument("Key is null or empty.");
    }

    if (length > dataStor_->GetMaxValueLength()) {
        return Status::NotSupported("Data length cann't be longer than max segment size");
    }

    KVSlice slice(key, key_len, data, length);

    Status s = dataStor_->WriteData(slice);

    if (s.ok()) {
        if(!options_.disable_cache && !slice.GetDataLen()){
            rdCache_->Put(slice.GetKeyStr(),slice.GetDataStr());
	    //rdMap.insert(pair<string,string>(slice.GetKeyStr(), slice.GetDataStr() ) );
        }
    }

    return s;
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

    if(!options_.disable_cache) {
	/*map<string, string>::iterator iter;
	iter = rdMap.find(slice.GetKeyStr()); 
	if(iter!=rdMap.end()){
		data = iter->second; 
		return Status::OK();
	}*/

        if(rdCache_->Get(slice.GetKeyStr(), data)) {
            //rdCache_->Put(slice.GetKeyStr(), data);
            return Status::OK();
        }
   }

    res = idxMgr_->GetHashEntry(&slice);
    if (!res) {
        //The key is not exist
        return Status::NotFound("Key is not found.");
    }

    Status s = dataStor_->ReadData(slice, data);
    //Status s = Status::OK();
    if (s.ok()) {
        if(!options_.disable_cache) {
            rdCache_->Put(slice.GetKeyStr(), data);
	//rdMap.insert(pair<string,string>(slice.GetKeyStr(), data));
        }
    }

    return s;
}

Status KVDS::InsertBatch(WriteBatch *batch) {
    if (batch->batch_.empty()) {
        return Status::OK();
    }

    Status s = dataStor_->WriteBatchData(batch);

    if (s.ok()) {
        if(!options_.disable_cache) {
            for (std::list<KVSlice *>::iterator iter = batch->batch_.begin();
                    iter != batch->batch_.end(); iter++) {
                if(!(*iter)->GetDataLen()) {//no "" should be put in to the cache
                    rdCache_->Put((*iter)->GetKeyStr(), (*iter)->GetDataStr());
                }
            }
        }
    }

    return s;
}

void KVDS::Do_GC() {
    dataStor_->Do_GC();
}

void KVDS::ClearReadCache() {
    bdev_->ClearReadCache();
}

}

