//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <string>
#include <iostream>
#include "test_base.h"

class TestDb : public TestBase {
public:
    double insert(KvdbDS *db) {
        //insert something
        string test_key = "test_key";
        int test_key_size = 8;
        string test_value = "test_value";
        int test_value_size = 10;

        KVTime tv_start;
        Status s = db->Insert(test_key.c_str(), test_key_size,
                              test_value.c_str(), test_value_size);
        EXPECT_TRUE(s.ok());
        KVTime tv_end;
        double diff_time = (tv_end - tv_start) / 1000.0;

        cout << "cost time: " << diff_time << "ms" << endl;
        return diff_time;
    }
};

TEST_F(TestDb,uninitializeBlockDevice)
{

    KvdbDS *db = KvdbDS::Create_KvdbDS("/dev/loop3", opts);

    EXPECT_EQ(NULL,db);
}

TEST_F(TestDb,reopendb)
{
    //Options opts;
    string path="/dev/loop2";
    KvdbDS *db = KvdbDS::Create_KvdbDS(path.c_str(), opts);

    delete db;
    KvdbDS::Open_KvdbDS(path.c_str(), opts);

    KvdbDS* db2=KvdbDS::Open_KvdbDS(path.c_str(), opts);
    EXPECT_FALSE(NULL==db2);

    delete db2;
}

TEST_F(TestDb,usedbwithoutdeleting)
{
    string path="/dev/loop2";
    KvdbDS *db = KvdbDS::Create_KvdbDS(path.c_str(), opts);

    insert(db);
    //delete db;
    //KvdbDS::Open_KvdbDS(path.c_str(), opts);
    //throw exception
    //Floating point exception (core dumped)
}

//this option has default value, but improper passed value still cause unhandled exception
TEST_F(TestDb,zerosegmentsize)
{
    opts.segment_size=0;
    string path="/dev/loop2";
    KvdbDS *db = KvdbDS::Create_KvdbDS(path.c_str(), opts);

    EXPECT_TRUE(NULL==db);

    //unhadled exception
    //Floating point exception (core dumped)
}

//user should pass a value of hashtable size
TEST_F(TestDb,zerohashtablesize)
{
    opts.hashtable_size=0;
    string path="/dev/loop2";
    KvdbDS *db = KvdbDS::Create_KvdbDS(path.c_str(), opts);

    EXPECT_FALSE(NULL==db);

    //pass, should be failed
}

TEST_F(TestDb,zeroexpiretime)
{
    opts.expired_time=0;
    string path="/dev/loop2";
    KvdbDS *db = KvdbDS::Create_KvdbDS(path.c_str(), opts);

    EXPECT_FALSE(NULL==db);
    delete db;

    KvdbDS* db2=KvdbDS::Open_KvdbDS(path.c_str(), opts);
    EXPECT_FALSE(NULL==db2);

    insert(db2);
}

TEST_F(TestDb,expiretime)
{
    opts.expired_time=10000; // 10ms
    string path="/dev/loop2";
    KvdbDS *db = KvdbDS::Create_KvdbDS(path.c_str(), opts);

    EXPECT_FALSE(NULL==db);
    delete db;

    KvdbDS* db2=KvdbDS::Open_KvdbDS(path.c_str(), opts);
    EXPECT_FALSE(NULL==db2);

    double time=insert(db2);
    EXPECT_GT(time,10);
}

TEST_F(TestDb,seg_full_rate)
{
    //gc relative
}

TEST_F(TestDb,gc_upper_level)
{
    //gc relative
}

TEST_F(TestDb,gc_lower_level)
{
    //gc relative
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();

}

