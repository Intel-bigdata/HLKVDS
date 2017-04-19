/*
 * test_micro_perf.cc
 *
 *  Created on: Apr 19, 2017
 *      Author: yongfu
 */

#include "test_base.h"

class TestMicroPerf : public TestBase {

};

void Create_Keys(int record_num, vector<string> &key_list) {
    for (int index = 0; index < record_num; index++) {
        char c_key[11] = "kkkkkkkkkk";
        stringstream key_ss;
        key_ss << index;
        string key(key_ss.str());
        memcpy(c_key, key.c_str(), key.length());
        string key_last = c_key;
        key_list.push_back(key_last);
    }
}

/*TEST_F(TestMicroPerf,getLargeRecords)
{
    string filename="/dev/loop2";
    KvdbDS *db = KvdbDS::Open_KvdbDS(filename.c_str(), opts);

    vector<string> key_list;
    int records=10000;
    Create_Keys(records, key_list);
    string value = string(4096, 'v');

    Status s;
    KVTime tv_start;
    for (int i = 0; i < 100 ; i++) {
        string key = key_list[i];
        string get_data;

        s = db->Get(key.c_str() ,10, get_data);

        if (!s.ok()) {
            cout << "Get key = " << key << "from DB failed!" << endl;
        }
        if (strcmp(get_data.c_str(), value.c_str()) != 0) {
            cout << "Get key=" << key << "Inconsistent! " << endl;
        }
    }
    KVTime tv_end;
    std::cout<<"cost time:" <<(tv_end - tv_start)<<std::endl;

    delete db;

}*/

TEST_F(TestMicroPerf,iteratorLargeRecords)
{
    string filename="/dev/loop2";
    KvdbDS *db = KvdbDS::Open_KvdbDS(filename.c_str(), opts);
    KvdbIter *iter=db->newIterator(db);

    string value = string(4096, 'v');
    Status s;

    iter->SeekToFirst();
    KVTime tv_start;
    while(true){
        Status s=iter->Next();
        if(!s.ok()){
            break;
        }
        if(!iter->Valid()){
            break;
        }
        if (strcmp(iter->Value().c_str(), value.c_str()) != 0) {
            cout << "Get key=" << iter->Key()<< "Inconsistent! " << endl;
        }
        std::cout<<"key:"<<iter->Key()<<",value:"<<iter->Value()<<std::endl;
    }

    KVTime tv_end;
    std::cout<<"cost time:" <<(tv_end - tv_start)<<std::endl;

    delete db;

}

int main(int argc, char** argv) {
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();

}
