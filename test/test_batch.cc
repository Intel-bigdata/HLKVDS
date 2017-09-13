#include "test_base.h"

class TestBatch : public TestBase {
};

TEST_F(TestBatch,batch)
{
    int db_size = 100;
    KVDS *db = Create_DB(db_size);

    string test_key = "test-key";
    int test_key_size = 8;
    string test_value = "test-value";
    int test_value_size = 10;

    WriteBatch writeBatch;

    writeBatch.put(test_key.c_str(), test_key_size, test_value.c_str(),
                          test_value_size);
    string test_key2="testkey2";
    writeBatch.put(test_key2.c_str(), test_key_size, test_value.c_str(),
                    test_value_size);

    Status s=db->InsertBatch(&writeBatch);
    EXPECT_TRUE(s.ok());

    string get_data;
    s=db->Get(test_key.c_str(), test_key_size, get_data);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(test_value,get_data);

    get_data="";
    s=db->Get(test_key2.c_str(), test_key_size, get_data);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(test_value,get_data);

    db->printDbStates();

}
TEST_F(TestBatch,emptyBatch)
{
    int db_size = 100;
    KVDS *db = Create_DB(db_size);
    //int total1=db->segMgr_->GetTotalFreeSegs();
    WriteBatch writeBatch;
    Status s=db->InsertBatch(&writeBatch);
    EXPECT_TRUE(s.ok());
    db->printDbStates();//don't write actually
    //int total2= db->segMgr_->GetTotalFreeSegs();
    //EXPECT_EQ(total1,total2);
}

TEST_F(TestBatch,update)
{
    int db_size = 100;
    KVDS *db = Create_DB(db_size);

    string test_key = "test-key";
    int test_key_size = 8;
    string test_value = "test-value";
    int test_value_size = 10;

    WriteBatch writeBatch;

    writeBatch.put(test_key.c_str(), test_key_size, test_value.c_str(),
                          test_value_size);
    string new_value="anew-value";
    writeBatch.put(test_key.c_str(), test_key_size, new_value.c_str(),
                    test_value_size);
    Status s = db->InsertBatch(&writeBatch);
    EXPECT_TRUE(s.ok());

    string get_data;
    s=db->Get(test_key.c_str(), test_key_size, get_data);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(new_value,get_data);
}

TEST_F(TestBatch,deleteInBatch)
{
    int db_size = 100;
    KVDS *db = Create_DB(db_size);

    string test_key = "test-key";
    int test_key_size = 8;
    string test_value = "test-value";
    int test_value_size = 10;

    WriteBatch writeBatch;

    writeBatch.put(test_key.c_str(), test_key_size, test_value.c_str(),
                          test_value_size);
    writeBatch.del(test_key.c_str(), test_key_size);
    Status s;

    s = db->InsertBatch(&writeBatch);
    EXPECT_TRUE(s.ok());

    string get_data;
    s=db->Get(test_key.c_str(), test_key_size, get_data);    cout<<s.ToString()<<endl;
    //EXPECT_TRUE(s.notfound());
    //EXPECT_EQ("",get_data);
}


//TEST_F(TestBatch,largeBatch)
//{
//    int db_size = 100;
//    KVDS *db = Create_DB(db_size);
//
//    WriteBatch writeBatch;
//
//    int test_key_size = 8;
//    int test_value_size = 100;
//    string test_value = string(test_value_size, 'v');
//
//    string *str_key;
//
//    for (int i = 0; i < 3000; i++) {
//        stringstream ss;
//        string str=string("");
//        ss << i;
//        ss >> str;
//
//        string test_key="key_";
//        test_key = test_key + str;
//
//        str_key=new string(test_key);
//        //std::cout<<"key:"<<*str_key<<std::endl;
//        writeBatch.put(str_key->c_str(), test_key_size, test_value.c_str(),
//                        test_value_size);
//
//    }
//    Status s = db->Write(&writeBatch);
//    EXPECT_FALSE(s.ok());
//    std::cout<<s.ToString()<<std::endl;
//    db->printDbStates();
//
//    delete str_key;
//}
//
//TEST_F(TestBatch,mixedBatchAndSingleOps)
//{
//    int db_size=100;
//    KVDS *db= Create_DB(db_size);
//
//    string test_key = "test-key";
//    int test_key_size = 8;
//    string test_value = "test-value";
//    int test_value_size = 10;
//
//    Status s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
//    EXPECT_TRUE(s.ok());
//
//    WriteBatch writeBatch;
//    string test_key2="testkey2"; // add a new key-value
//    writeBatch.put(test_key2.c_str(), test_key_size, test_value.c_str(),
//                          test_value_size);
//    string new_value="anew-value"; //update value in batch
//    writeBatch.put(test_key.c_str(), test_key_size, new_value.c_str(),
//                    test_value_size);
//    s = db->Write(&writeBatch);
//    EXPECT_TRUE(s.ok());
//
//    string get_data;
//    s=db->Get(test_key.c_str(), test_key_size, get_data);
//    EXPECT_EQ(new_value,get_data);
//
//    get_data.clear();
//    s=db->Get(test_key2.c_str(), test_key_size, get_data);
//    EXPECT_EQ(test_value,get_data);
//}



int main(int argc, char** argv) {
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();

}

