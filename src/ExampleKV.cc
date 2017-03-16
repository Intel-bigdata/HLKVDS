//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include "hyperds/Kvdb.h"

#define TEST_RECORD_NUM 1

#define TEST_DB_FILENAME "/dev/sdb1"
//#define TEST_DB_FILENAME "/dev/sdb3"

void Get(kvdb::DB *db) {
    int key_len;
    for (int index = 0; index < TEST_RECORD_NUM; index++) {
        stringstream key_ss;
        key_ss << index;
        std::string key(key_ss.str());
        key_len = key.length();
        std::string get_data;
        db->Get(key.c_str(), key_len, get_data);
        std::cout << "Get Item Success. key = " << key << " ,value = "
                << get_data << std::endl;
    }

}
void Insert(kvdb::DB *db) {
    int key_len;
    int value_len;
    for (int index = 0; index < TEST_RECORD_NUM; index++) {
        stringstream key_ss;
        key_ss << index;
        std::string key(key_ss.str());
        std::string value = "Begin" + key + "End";
        key_len = key.length();
        value_len = value.length();
        db->Insert(key.c_str(), key_len, value.c_str(), value_len);
        std::cout << "Insert Item Success. key = " << key << " ,value = "
                << value << std::endl;
    }

}

void Delete(kvdb::DB *db) {
    int key_len;
    for (int index = 0; index < TEST_RECORD_NUM; index++) {
        stringstream key_ss;
        key_ss << index;
        std::string key(key_ss.str());
        key_len = key.length();
        db->Delete(key.c_str(), key_len);
        std::cout << "Delete Item Success. key = " << key << std::endl;
    }

}
void OpenExample() {
    kvdb::DB *db;

    if (!kvdb::DB::OpenDB(TEST_DB_FILENAME, &db)) {
        return;
    }

    Get(db);
    Insert(db);
    sleep(2);
    Get(db);
    Delete(db);
    sleep(2);
    Get(db);
    Insert(db);
    sleep(2);

    delete db;
}

int main() {
    OpenExample();
    return 0;
}
