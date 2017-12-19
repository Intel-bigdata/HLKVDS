#include <stdlib.h>
#include <boost/algorithm/string.hpp>

#include "Kvdb_Impl.h"
#include "KvdbIter.h"
#include "Db_Structure.h"

#include "BlockDevice.h"
#include "KernelDevice.h"
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

    KVDS* kvds = new KVDS(filename, opts);

    if (!kvds->openAllDevices(kvds->paths_)) {
        delete kvds;
        return NULL;
    }

    int datastor_type = opts.datastor_type;
    kvds->dataStor_ = DataStor::Create(kvds->options_, kvds->bdVec_, kvds->sbMgr_, kvds->idxMgr_, datastor_type);

    if (kvds->dataStor_ == NULL) {
        delete kvds;
        return NULL;
    }

    kvds->idxMgr_->InitDataStor(kvds->dataStor_);
    kvds->metaStor_->InitDataStor(kvds->dataStor_);

    if (!kvds->metaStor_->CreateMetaData()) {
        delete kvds;
        return NULL;
    }

    __INFO("\nCreate KVDS Success!!!");
    kvds->printDbStates();

    kvds->dataStor_->CreateAllSegments();
    kvds->startThds();

    kvds->isOpen_ = true;

    return kvds;

}

Iterator* KVDS::NewIterator() {
    return new KvdbIter(idxMgr_, dataStor_);
}

void KVDS::printDbStates() {
    sbMgr_->printSBInfo();
    dataStor_->printDeviceTopologyInfo();

    idxMgr_->printDynamicInfo();
    dataStor_->printDynamicInfo();
}

KVDS* KVDS::Open_KVDS(const char* filename, Options opts) {
    KVDS *kvds = new KVDS(filename, opts);

    Status s = kvds->openDB();
    if (!s.ok()) {
        delete kvds;
        return NULL;
    }

    kvds->isOpen_ = true;
    return kvds;

}

Status KVDS::openDB() {
    if (!openAllDevices(paths_)) {
        return Status::IOError("Could not open all device");
    }

    int datastor_type = -1;
    if ( !metaStor_->TryLoadSB(datastor_type)) {
        return  Status::IOError("Could not load SB");
    }
    dataStor_ = DataStor::Create(options_, bdVec_, sbMgr_, idxMgr_, datastor_type);

    if (dataStor_ == NULL) {
        return Status::NotSupported("Could not support this data store");
    }

    idxMgr_->InitDataStor(dataStor_);
    metaStor_->InitDataStor(dataStor_);

    if (!metaStor_->LoadMetaData()) {
        return Status::IOError("Could not read meta data");
    }

    __INFO("\nOpen KVDS Success!!!");
    printDbStates();

    dataStor_->CreateAllSegments();
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
    if (isOpen_) {
        closeDB();
    }
    delete idxMgr_;
    delete sbMgr_;
    if(!options_.disable_cache){
        delete rdCache_;
    }
    delete metaStor_;
    if (dataStor_) {
        delete dataStor_;
    }
    closeAllDevices();
}

bool KVDS::openAllDevices(string paths) {

    vector<string> fields;
    boost::split(fields, paths, boost::is_any_of(FileDelim));
    vector<string>::iterator iter;
    for(iter = fields.begin(); iter != fields.end(); iter++){
        BlockDevice *bdev = new KernelDevice();

        if (bdev->Open(*iter) < 0) {
            return false;
        }__DEBUG("Open Device %s Success!", (*iter).c_str());

        bdVec_.push_back(bdev);
    }
    return true;
}

void KVDS::closeAllDevices() {
    vector<BlockDevice *>::iterator iter;
    for ( iter = bdVec_.begin() ; iter != bdVec_.end(); ) {
        BlockDevice *bdev = *iter;
        delete bdev;
        iter = bdVec_.erase(iter);
    }
}

KVDS::KVDS(const char* filename, Options opts) :
    paths_(string(filename)), sbMgr_(NULL), idxMgr_(NULL), rdCache_(NULL), metaStor_(NULL), dataStor_(NULL), options_(opts), isOpen_(false) {

    sbMgr_ = new SuperBlockManager(options_);
    idxMgr_ = new IndexManager(sbMgr_, options_);

    if(!options_.disable_cache){
	if(options_.is_segmented){
	    rdCache_ = new ReadCacheTable( options_.is_dedup, CachePolicy(options_.cache_policy), (size_t) options_.hash_code, (size_t) options_.cache_size, options_.slru_percent);
	}else{
	    rdCache_ = new ReadCacheSegment( options_.is_dedup, CachePolicy(options_.cache_policy), (size_t) options_.cache_size, options_.slru_percent);
		
	}
    }

    metaStor_ = new MetaStor(filename, bdVec_, sbMgr_, idxMgr_, options_);
}

Status KVDS::Insert(const char* key, uint32_t key_len, const char* data,
                      uint16_t length) {
    if (key == NULL || key[0] == '\0') {
        return Status::InvalidArgument("Key is null or empty.");
    }

    KVSlice slice(key, key_len, data, length);

    Status s = dataStor_->WriteData(slice);

    if (s.ok()) {
        if(!options_.disable_cache && !slice.GetDataLen()){
            rdCache_->Put(slice.GetKeyStr(),slice.GetDataStr());
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
        if(rdCache_->Get(slice.GetKeyStr(), data)) {
           // rdCache_->Put(slice.GetKeyStr(), data);
            return Status::OK();
        }
    }

    res = idxMgr_->GetHashEntry(&slice);
    if (!res) {
        //The key is not exist
        return Status::NotFound("Key is not found.");
    }

    Status s = dataStor_->ReadData(slice, data);

    if (s.ok()) {
        if(!options_.disable_cache) {
            rdCache_->Put(slice.GetKeyStr(), data);
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
    dataStor_->ManualGC();
}

void KVDS::ClearReadCache() {
    vector<BlockDevice*>::iterator iter;
    for (iter = bdVec_.begin(); iter != bdVec_.end(); iter++) {
        BlockDevice *bdev = *iter;
        bdev->ClearReadCache();
    }
}

}

