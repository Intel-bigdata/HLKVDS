/*
 * test_batch.cc
 *
 *  Created on: Mar 29, 2017
 *      Author: yongfu
 */
#include "test_base.h"

class TestIterator : public TestBase {
public:
    int db_size = 100;
    KvdbDS *db = Create_DB(db_size);

    WriteBatch writeBatch;

    int test_key_size = 10;
    int test_value_size = 10;
    string test_value ="test-value";

    string *str_key;
    virtual void SetUp() {
        for (int i = 0; i < 100; i++) {
            stringstream ss;
            string str = string("");
            ss << i;
            ss >> str;
            string test_key = "test-key";
            test_key = test_key + str;

            str_key = new string(test_key);
            writeBatch.put(str_key->c_str(), test_key_size, test_value.c_str(),
                           test_value_size);

        }
        Status s = db->Write(&writeBatch);
        EXPECT_TRUE(s.ok());
    }
};

TEST_F(TestIterator,seektofisrt)
{
    KvdbIter *iter=db->newIterator(db);
    Status s=iter->SeekToFirst();
    if(s.ok()&&iter->Valid()){
        std::cout<<"key:"<<iter->Key()<<",value:"<<iter->Value()<<std::endl;
        //EXPECT_EQ("test-key2",iter->Key());
        EXPECT_EQ(test_value,iter->Value());
    }else{
        EXPECT_TRUE(false);
    }
    delete iter;
}


TEST_F(TestIterator,seektolast)
{
    KvdbIter *iter=db->newIterator(db);
    Status s=iter->SeekToLast();
    if(!s.ok()||!iter->Valid()){
        EXPECT_TRUE(false);
    }

    std::cout<<"key:"<<iter->Key()<<",value:"<<iter->Value()<<std::endl;
    //EXPECT_EQ("test-key6",iter->Key());
    EXPECT_EQ(test_value,iter->Value());
    delete iter;

}

TEST_F(TestIterator,next)
{
    KvdbIter *iter=db->newIterator(db);
    iter->SeekToFirst();
    while(true){
        Status s=iter->Next();
        if(!s.ok()){
            break;
        }
        if(!iter->Valid()){
            break;
        }
        std::cout<<"key:"<<iter->Key()<<",value:"<<iter->Value()<<std::endl;
    }

    delete iter;

}

TEST_F(TestIterator,prev)
{
    KvdbIter *iter=db->newIterator(db);
    iter->SeekToLast();
    while(true){
        Status s=iter->Prev();
        if(!s.ok()){
            break;
        }
        if(!iter->Valid()){
            break;
        }
        std::cout<<"key:"<<iter->Key()<<",value:"<<iter->Value()<<std::endl;
    }

}


TEST_F(TestIterator,seek)
{
    const char* key="test-key4";
    KvdbIter *iter=db->newIterator(db);
    Status s=iter->Seek(key);

    if(s.ok()&&iter->Valid()){
        EXPECT_EQ(key,iter->Key());
        EXPECT_EQ(test_value,iter->Value());
    }else{
        EXPECT_TRUE(false);
    }
}



int main(int argc, char** argv) {
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();

}

