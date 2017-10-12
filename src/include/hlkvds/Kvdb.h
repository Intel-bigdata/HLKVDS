#ifndef _HLKVDS_KVDB_H_
#define _HLKVDS_KVDB_H_

#include <iostream>
#include <string>

#include "hlkvds/Options.h"
#include "hlkvds/Status.h"
#include "hlkvds/Write_batch.h"
#include "hlkvds/Iterator.h"

using namespace std;

namespace hlkvds {

class KVDS;

class DB {
public:
    static bool CreateDB(string filename, Options opts = Options());
    static bool OpenDB(string filename, DB** db, Options opts = Options());

    virtual ~DB();

    Status Insert(const char* key, uint32_t key_len, const char* data,
                uint16_t length);
    Status Delete(const char* key, uint32_t key_len);
    Status Get(const char* key, uint32_t key_len, string &data);

    Status InsertBatch(WriteBatch *batch);
    Iterator* NewIterator();

    void Do_GC();
    void printDbStates();

private:
    DB();
    DB(const DB &);
    DB& operator=(const DB &);

//singleton
    class Helper
    {
    public:
        Helper();
 
        ~Helper();
    };
    friend class Helper;
    static Helper helper_;

    string fileName_;
    KVDS *kvds_;
    static DB *instance_;
};

} // namespace hlkvds

#endif //_HLKVDS_KVDB_H_
