#include <string>
#include <iostream>

#include "../src/Kvdb_Impl.h"
#include "../src/Options.h"
#include "gtest/gtest.h"


using namespace std;
using namespace kvdb;

#define FILENAME  "/dev/loop2"
//#define FILENAME  "/dev/sdb1"
#define DB_SIZE 512
#define RECORDS 500


class test_operations: public ::testing::Test{

};

int Create_DB(string filename, int db_size)
{
    int ht_size = db_size ;
    int segment_size = SEGMENT_SIZE;

    Options opts;
    opts.hashtable_size = ht_size;
    opts.segment_size = segment_size;

    KVTime tv_start;
    KvdbDS *db = KvdbDS::Create_KvdbDS(filename.c_str(), opts);

    KVTime tv_end;
    double diff_time = (tv_end - tv_start) / 1000000.0;

    cout << "Create DB use time: " << diff_time << "s" << endl;
    delete db;
    db = NULL;
    return 0;
}

KvdbDS* initDb(int db_size)
{		
	 EXPECT_TRUE(Create_DB(FILENAME, db_size) >= 0);

	  Options opts;
	  return KvdbDS::Open_KvdbDS(FILENAME, opts);

}


TEST_F(test_operations,insert)
{
	int db_size=100;
	KvdbDS *db= initDb(db_size);
 
    string test_key = "test-key";
    int test_key_size = 8;
    string test_value = "test-value";
    int test_value_size = 10;

    EXPECT_TRUE(db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size));
	string get_data;
    EXPECT_TRUE(db->Get(test_key.c_str(), test_key_size, get_data));

   	EXPECT_EQ(test_value,get_data);

	delete db;
}

TEST_F(test_operations,emtpyvalue)
{
	int db_size=100;
	KvdbDS *db= initDb(db_size);

    string test_key = "";
    int test_key_size = 8;
    string test_value = "";
    int test_value_size = 10;

    EXPECT_TRUE(db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size));
	string get_data;
    EXPECT_TRUE(db->Get(test_key.c_str(), test_key_size, get_data));

   	EXPECT_EQ(test_value,get_data);

	delete db;
}

TEST_F(test_operations,zerosize)
{
	int db_size=100;
	KvdbDS *db= initDb(db_size);

    string test_key = "";
    int test_key_size = 0;
    string test_value = "";
    int test_value_size = 0;

    EXPECT_TRUE(db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size));
	string get_data;
    EXPECT_TRUE(db->Get(test_key.c_str(), test_key_size, get_data));

   	EXPECT_EQ(test_value,get_data);

	delete db;
}


TEST_F(test_operations,bigsizevalue)
{
	int db_size=100;
	KvdbDS *db= initDb(db_size);

    string test_key = "test_key";
    int test_key_size = 8;

	string test_value = string(1024000, 'v');

    int test_value_size =1024000;
	

    EXPECT_TRUE(db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size));

	//db->readMetaDataFromDevice();//TODO databases status dump
	string get_data;
    EXPECT_TRUE(db->Get(test_key.c_str(), test_key_size, get_data));

   	EXPECT_EQ(test_value,get_data);

	delete db;
}

TEST_F(test_operations,wrongvaluesize)
{
	int db_size=100;
	KvdbDS *db= initDb(db_size);

    string test_key = "test_key";
    int test_key_size = 8;

	string test_value = string(1024000, 'v');

    int test_value_size =1024;
	

    EXPECT_TRUE(db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size));

	string get_data;
    EXPECT_TRUE(db->Get(test_key.c_str(), test_key_size, get_data));

   	EXPECT_EQ(test_value,get_data);

	delete db;
}

TEST_F(test_operations,insertmorethandbsize)
{
	int db_size=100;
	KvdbDS *db= initDb(db_size);

    string test_key = "key_";
    int test_key_size = 8;

	string test_value = "test_value";
    int test_value_size =1024;
	
	for(int i=0;i<100;i++)
	{	
		stringstream ss;
    	string str;
    	ss<<i;
    	ss>>str;
		test_key=test_key+str;
		EXPECT_TRUE(db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size));
	}
		
    EXPECT_FALSE(db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size));


	delete db;
}



TEST_F(test_operations,updatevalue)
{
	int db_size=100;
	KvdbDS *db= initDb(db_size);

    string test_key = "test_key";
    int test_key_size = 8;

	string test_value = "test_value";
    int test_value_size =10;
	

    EXPECT_TRUE(db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size));

	string get_data;
    EXPECT_TRUE(db->Get(test_key.c_str(), test_key_size, get_data));

   	EXPECT_EQ(test_value,get_data);


	test_value = "test-value-new";
    test_value_size = 14;

	get_data="";
    EXPECT_TRUE(db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size));
	EXPECT_TRUE(db->Get(test_key.c_str(), test_key_size, get_data));
	EXPECT_EQ(test_value,get_data);
    

	delete db;
}

TEST_F(test_operations,deletekey)
{
	int db_size=100;
	KvdbDS *db= initDb(db_size);

    string test_key = "test_key";
    int test_key_size = 8;
	string test_value = "test_value";
    int test_value_size =10;
	

    EXPECT_TRUE(db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size));

	string get_data;
    EXPECT_TRUE(db->Get(test_key.c_str(), test_key_size, get_data));
   	EXPECT_EQ(test_value,get_data);

	EXPECT_TRUE(db->Delete(test_key.c_str(), test_key_size));
	get_data="";
	EXPECT_FALSE(db->Get(test_key.c_str(), test_key_size, get_data));

	delete db;
}


TEST_F(test_operations,updateafterdelete)
{
	int db_size=100;
	KvdbDS *db= initDb(db_size);

    string test_key = "test_key";
    int test_key_size = 8;
	string test_value = "test_value";
    int test_value_size =10;
	

    EXPECT_TRUE(db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size));

	string get_data;
    EXPECT_TRUE(db->Get(test_key.c_str(), test_key_size, get_data));
   	EXPECT_EQ(test_value,get_data);

	EXPECT_TRUE(db->Delete(test_key.c_str(), test_key_size));
	get_data="";
	EXPECT_FALSE(db->Get(test_key.c_str(), test_key_size, get_data));

	EXPECT_TRUE(db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size));
	get_data="";
    EXPECT_TRUE(db->Get(test_key.c_str(), test_key_size, get_data));
   	EXPECT_EQ(test_value,get_data);
	

	delete db;
}

TEST_F(test_operations,deleteagain)
{
	int db_size=100;
	KvdbDS *db= initDb(db_size);

    string test_key = "test_key";
    int test_key_size = 8;
	string test_value = "test_value";
    int test_value_size =10;
	

    EXPECT_TRUE(db->Insert(test_key.c_str(), test_key_size, test_value.c_str(), test_value_size));

	string get_data;
    EXPECT_TRUE(db->Get(test_key.c_str(), test_key_size, get_data));
   	EXPECT_EQ(test_value,get_data);

	EXPECT_TRUE(db->Delete(test_key.c_str(), test_key_size));
	get_data="";
	EXPECT_FALSE(db->Get(test_key.c_str(), test_key_size, get_data));

	EXPECT_TRUE(db->Delete(test_key.c_str(), test_key_size));
	get_data="";
	EXPECT_FALSE(db->Get(test_key.c_str(), test_key_size, get_data));

	delete db;
}


TEST_F(test_operations,concurrentinsertwithsamekey)
{

}



int main(int argc, char** argv){
	::testing::InitGoogleTest(&argc, argv);
	 return RUN_ALL_TESTS();

}
