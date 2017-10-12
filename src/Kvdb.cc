#include "hlkvds/Kvdb.h"
#include "Kvdb_Impl.h"

namespace hlkvds {
DB* DB::instance_ = NULL;
typename DB::Helper DB::helper_;

DB::Helper::Helper(){
     instance_ = new DB();
}

DB::Helper::~Helper(){
     delete instance_;
     instance_ = NULL;
}

bool DB::CreateDB(string filename, Options opts) {
    KVDS* new_kvds;
    new_kvds = KVDS::Create_KVDS(filename.c_str(), opts);
    if (!new_kvds) {
        std::cout << "CreateDB failed" << std::endl;
        return false;
    }

    delete new_kvds;
    return true;
}

bool DB::OpenDB(string filename, DB** db, Options opts) {
    /*if (instance_ == NULL) {
        instance_ = new DB();
    }*/

    instance_->kvds_ = KVDS::Open_KVDS(filename.c_str(), opts);
    if (!instance_->kvds_) {
        std::cout << "OpenDB failed" << std::endl;
        return false;
    }

    *db = instance_;
    //std::cout << "OpenDB success" <<std::endl;
    return true;
}

DB::DB() {
}

DB::~DB() {/*
    if (instance_) {
        delete instance_->kvds_;
        instance_->kvds_ = NULL;
    }
    instance_ = NULL;*/
}

Status DB::Insert(const char* key, uint32_t key_len, const char* data,
                uint16_t length) {
    Status s = kvds_->Insert(key, key_len, data, length);
    if (!s.ok()) {
        std::cout << "DB Insert failed" << std::endl;
    }

    return s;
}

Status DB::Delete(const char* key, uint32_t key_len) {
    Status s = kvds_->Delete(key, key_len);
    if (!s.ok()) {
        std::cout << "DB Delete failed" << std::endl;
    }
    return s;
}

Status DB::Get(const char* key, uint32_t key_len, string &data) {
    Status s = kvds_->Get(key, key_len, data);
    if (!s.ok()) {
        std::cout << "DB Get failed" << std::endl;
    }
    return s;
}

void DB::Do_GC() {
    kvds_->Do_GC();
}

Status DB::InsertBatch(WriteBatch *batch) {
    return kvds_->InsertBatch(batch);
}

Iterator* DB::NewIterator() {
    return kvds_->NewIterator();
}

void DB::printDbStates()
{
    return kvds_->printDbStates();
}

}// end namespace hlkvds
