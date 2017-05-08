/*
 * test_micro_perf.cc
 *
 *  Created on: Apr 19, 2017
 *      Author: yongfu
 */

#include "test_base.h"

class TestMicroPerf : public TestBase {
public:
    string filename="/dev/loop2";
    int db_size=100;
    int test_key_size = 10;
    int test_value_size =1024;
    string test_value = string(test_value_size, 'v');

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
};



TEST_F(TestMicroPerf,insertdirectly)
{
    KvdbDS *db= Create_DB(db_size);

    vector<string> key_list;
    Create_Keys(db_size, key_list);

    Status s;
    KVTime tv_start;
    for (int i = 0; i < db_size ; i++) {
        s=db->Insert(key_list[i].c_str(), test_key_size, test_value.c_str(), test_value_size);
        EXPECT_TRUE(s.ok());
    }

    KVTime tv_end;
    std::cout<<"cost time:" <<(tv_end - tv_start)<<std::endl;

    db->printDbStates();
    delete db;
}
TEST_F(TestMicroPerf,insertbatch)
{
    KvdbDS *db = Create_DB(db_size);

    WriteBatch writeBatch;

    vector<string> key_list;
    Create_Keys(db_size, key_list);

    KVTime tv_start;

    for (int i = 0; i < db_size ; i++) {
        writeBatch.put(key_list[i].c_str(), test_key_size, test_value.c_str(),
                       test_value_size);
    }

    Status s = db->Write(&writeBatch);
    EXPECT_TRUE(s.ok());
    KVTime tv_end;
    std::cout<<"cost time:" <<(tv_end - tv_start)<<std::endl;

    db->printDbStates();
    delete db;
}

TEST_F(TestMicroPerf,getLargeRecords)
{

    KvdbDS *db = KvdbDS::Open_KvdbDS(filename.c_str(), opts);

    vector<string> key_list;
    Create_Keys(db_size, key_list);

    Status s;
    KVTime tv_start;
    int count=0;
    for (int i = 0; i < db_size ; i++) {
        string key = key_list[i];
        string get_data;

        s = db->Get(key.c_str() ,10, get_data);

        if (!s.ok()) {
            cout << "Get key = " << key << "from DB failed!" << endl;
        }
        if (strcmp(get_data.c_str(), test_value.c_str()) != 0) {
            cout << "Get key=" << key << "Inconsistent! " << endl;
        }
        count++;
    }
    EXPECT_EQ(db_size,count);
    KVTime tv_end;
    std::cout<<"cost time:" <<(tv_end - tv_start)<<std::endl;

    delete db;

}

TEST_F(TestMicroPerf,iteratorLargeRecords)
{
    KvdbDS *db = KvdbDS::Open_KvdbDS(filename.c_str(), opts);
    KvdbIter *iter=db->newIterator(db);

    Status s;

    iter->SeekToFirst();
    KVTime tv_start;
    int count=0;
    while(true){
        Status s=iter->Next();
        if(!s.ok()){
            break;
        }
        if(!iter->Valid()){
            break;
        }
        if (strcmp(iter->Value().c_str(), test_value.c_str()) != 0) {
            cout << "Get key=" << iter->Key()<< "Inconsistent! " << endl;
        }

        count++;
    }

    EXPECT_EQ(db_size,count);
    KVTime tv_end;
    std::cout<<"cost time:" <<(tv_end - tv_start)<<std::endl;

    delete db;
}



int main(int argc, char** argv) {
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();

}
