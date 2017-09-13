#include <string>
#include <iostream>
#include "test_base.h"

class test_operations : public TestBase {

};

TEST_F(test_operations,insert)
{
    int db_size=100;
    KVDS *db= Create_DB(db_size);

    string test_key = "test-key";
    int test_key_size = 8;
    string test_value = "test-value";
    int test_value_size = 10;

    Status s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);

    db->printDbStates();
    EXPECT_TRUE(s.ok());
    string get_data;
    s=db->Get(test_key.c_str(), test_key_size, get_data);
    EXPECT_TRUE(s.ok());

    EXPECT_EQ(test_value,get_data);
    get_data.clear();
    delete db;
}

TEST_F(test_operations,emptykey)
{
    int db_size=100;
    KVDS *db= Create_DB(db_size);

    string test_key = "";
    int test_key_size = 8;
    string test_value = "test-value";
    int test_value_size = 10;

    Status s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
    EXPECT_FALSE(s.ok());

    delete db;
}

TEST_F(test_operations, nonexistkey)
{
    int db_size = 100;
    KVDS *db = Create_DB(db_size);

    string test_key = "test-key";
    int test_key_size = 8;
    string test_value = "test-value";

    string get_data;
    Status s=db->Get(test_key.c_str(), test_key_size, get_data);
    EXPECT_FALSE(s.ok());
    cout << s.ToString() << endl;

    delete db;
}

TEST_F(test_operations,zerosize)
{
    int db_size=100;
    KVDS *db= Create_DB(db_size);

    string test_key = "test_key";
    int test_key_size = 8;
    string test_value = "";
    int test_value_size = 0;

    Status s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
    EXPECT_TRUE(s.ok());
    string get_data;
    s=db->Get(test_key.c_str(), test_key_size, get_data);
    EXPECT_TRUE(s.notfound());

    EXPECT_EQ(test_value,get_data);

    delete db;
}

TEST_F(test_operations,bigsizevalue)
{
    int db_size=100;
    KVDS *db= Create_DB(db_size);

    string test_key = "test_key";
    int test_key_size = 8;

    string test_value = string(1024000, 'v');

    int test_value_size =1024000;

    Status s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
    EXPECT_TRUE(s.ok());

    db->printDbStates();
    string get_data;
    s=db->Get(test_key.c_str(), test_key_size, get_data);
    EXPECT_TRUE(s.ok());

    //std::cout<<"length:"<<get_data.length()<<std::endl;
    EXPECT_GT(test_value_size,get_data.length());

    delete db;
}

TEST_F(test_operations,wrongvaluesize)
{
    int db_size=100;
    KVDS *db= Create_DB(db_size);

    string test_key = "test_key";
    int test_key_size = 8;

    string test_value = string(1024000, 'v');

    int test_value_size =1024;

    Status s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
    EXPECT_TRUE(s.ok());

    string get_data;
    s=db->Get(test_key.c_str(), test_key_size, get_data);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(test_value_size,get_data.length());

    delete db;
}

TEST_F(test_operations,insertmorethandbsize)
{
    int db_size=100;//128 actually the db size should be equal to 128
    KVDS *db= Create_DB(db_size);

    string test_key = "key_";
    int test_key_size = 8;
    int test_value_size =102400;
    string test_value = string(test_value_size, 'v');

    Status s;

    for(int i=0;i<30;i++)
    {
        stringstream ss;
        string str;
        ss<<i;
        ss>>str;
        test_key=test_key+str;
        std::cout<<"key:"<<test_key<<std::endl;
        s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
        EXPECT_TRUE(s.ok());
        db->printDbStates();

        test_key = "key_";
    }

    s=db->Insert("key_last", test_key_size, test_value.c_str(), test_value_size);
    db->printDbStates();
    EXPECT_TRUE(s.ok());//TODO

    delete db;
}

TEST_F(test_operations,updatevalue)
{
    int db_size=100;
    KVDS *db= Create_DB(db_size);

    string test_key = "test_key";
    int test_key_size = 8;

    string test_value = "test_value";
    int test_value_size =10;

    Status s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
    EXPECT_TRUE(s.ok());

    test_value = "test-value-new";
    test_value_size = 14;

    s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
    EXPECT_TRUE(s.ok());

    string get_data="";
    s=db->Get(test_key.c_str(), test_key_size, get_data);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(test_value,get_data);

    delete db;
}

TEST_F(test_operations,deletekey)
{
    int db_size=100;
    KVDS *db= Create_DB(db_size);

    string test_key = "test_key";
    int test_key_size = 8;
    string test_value = "test_value";
    int test_value_size =10;

    Status s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
    EXPECT_TRUE(s.ok());

    s=db->Delete(test_key.c_str(), test_key_size);
    EXPECT_TRUE(s.ok());
    string get_data="";
    s=db->Get(test_key.c_str(), test_key_size, get_data);
    EXPECT_TRUE(s.notfound());
    EXPECT_EQ("",get_data);
    //std::cout<<"deleted key value:"<<get_data<<std::endl;

    delete db;
}

TEST_F(test_operations,updateafterdelete)
{
    int db_size=100;
    KVDS *db= Create_DB(db_size);

    string test_key = "test_key";
    int test_key_size = 8;
    string test_value = "test_value";
    int test_value_size =10;

    Status s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
    EXPECT_TRUE(s.ok());

    s=db->Delete(test_key.c_str(), test_key_size);
    EXPECT_TRUE(s.ok());

    s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
    EXPECT_TRUE(s.ok());
    string get_data="";
    s=db->Get(test_key.c_str(), test_key_size, get_data);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(test_value,get_data);

    delete db;
}

TEST_F(test_operations,deleteagain)
{
    int db_size=100;
    KVDS *db= Create_DB(db_size);

    string test_key = "test_key";
    int test_key_size = 8;
    string test_value = "test_value";
    int test_value_size =10;

    Status s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
    EXPECT_TRUE(s.ok());

    s=db->Delete(test_key.c_str(), test_key_size);
    EXPECT_TRUE(s.ok());

    std::cout<<"delete again."<<std::endl;
    s=db->Delete(test_key.c_str(), test_key_size);
    EXPECT_TRUE(s.ok());

    delete db;
}

TEST_F(test_operations,readAfterUpdateAndDelete)
{
    int db_size=100;
    KVDS *db= Create_DB(db_size);

    string test_key = "test_key";
    int test_key_size = 8;

    string test_value = "test_value";
    int test_value_size =10;

    Status s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
    EXPECT_TRUE(s.ok());

    string get_data;
    s=db->Get(test_key.c_str(), test_key_size, get_data);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(test_value,get_data);

    //update
    test_value = "test-value-new";
    test_value_size = 14;

    get_data="";
    s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
    EXPECT_TRUE(s.ok());

    s=db->Get(test_key.c_str(), test_key_size, get_data);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(test_value,get_data);

    //then delete
    s=db->Delete(test_key.c_str(), test_key_size);
    EXPECT_TRUE(s.ok());

    get_data="";
    //read again, value should be empty , or the raw one ?
    s=db->Get(test_key.c_str(), test_key_size, get_data);
    EXPECT_TRUE(s.notfound());
    EXPECT_EQ("",get_data);
}

TEST_F(test_operations,singlesegment)
{
    int db_size=100;
    KVDS *db= Create_DB(db_size);

    //std::cout<<"total used segments:"<<db->segMgr_->GetTotalUsedSegs()<<std::endl;;//TODO segMgr_ is private

    string test_key = "test_key";
    int test_key_size = 8;

    string test_value = "test_value";
    int test_value_size =10;

    Status s=db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size);
    EXPECT_TRUE(s.ok());
    //std::cout<<"total used segments:"<<db->segMgr_->GetTotalUsedSegs()<<std::endl;;

}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

}
