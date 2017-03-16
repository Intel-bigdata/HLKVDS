//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "hyperds/Options.h"
#include "Kvdb_Impl.h"

#define SEGMENT_SIZE 256 * 1024
#define KEY_LEN 10

#define TEST_BS 4096
//#define TEST_THREAD_NUM 2
#define TEST_THREAD_NUM 512

using namespace std;
using namespace kvdb;

enum Option {
    INSERT,
    GET
};

struct thread_arg {
    KvdbDS *db;
    int key_start;
    int key_end;
    vector<string> *key_list;
    string* data;
};

void usage() {
    fprintf(stderr, "Usage: Benchmark -f dbfile -s db_size -n num_records\n");
}

int Create_DB(string filename, int db_size) {
    int ht_size = db_size;
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

void Create_Keys(int record_num, vector<string> &key_list) {
    for (int index = 0; index < record_num; index++) {
        char c_key[KEY_LEN + 1] = "kkkkkkkkkk";
        stringstream key_ss;
        key_ss << index;
        string key(key_ss.str());
        memcpy(c_key, key.c_str(), key.length());
        string key_last = c_key;
        key_list.push_back(key_last);
    }
}

void* fun_insert(void *arg) {
    thread_arg *t_args = (thread_arg*) arg;
    KvdbDS *db = t_args->db;
    int key_start = t_args->key_start;
    int key_end = t_args->key_end;
    vector < string > &key_list = *t_args->key_list;

    string *value = t_args->data;

    //string value = string(TEST_BS, 'v');
    //int value_size = value.length();
    int value_size = TEST_BS;
    int key_len = KEY_LEN;
    Status s;

    for (int i = key_start; i < key_end + 1; i++) {
        string key = key_list[i];
        //string key = key_list[key_start];
        s = db->Insert(key.c_str(), key_len, value->c_str(), value_size);
        if (!s.ok()) {
            cout << "Insert key=" << key << "to DB failed!" << endl;
        }
    }
    return NULL;

}
void Bench_Insert(KvdbDS *db, int record_num, vector<string> &key_list) {

    cout << "Insert Bench Start, record_num = " << record_num
            << ", Please wait ..." << endl;

    string data = string(TEST_BS, 'v');

    thread_arg arglist[TEST_THREAD_NUM];
    pthread_t pidlist[TEST_THREAD_NUM];
    for (int i = 0; i < TEST_THREAD_NUM; i++) {
        int start = (record_num / TEST_THREAD_NUM) * i;
        int end = (record_num / TEST_THREAD_NUM) * (i + 1) - 1;
        arglist[i].db = db;
        arglist[i].key_start = start;
        arglist[i].key_end = end;
        arglist[i].key_list = &key_list;
        arglist[i].data = &data;
    }

    KVTime tv_start;
    for (int i = 0; i < TEST_THREAD_NUM; i++) {
        pthread_create(&pidlist[i], NULL, fun_insert, &arglist[i]);
    }

    for (int i = 0; i < TEST_THREAD_NUM; i++) {
        pthread_join(pidlist[i], NULL);
    }

    KVTime tv_end;
    double insert_time = (tv_end - tv_start) / 1000000.0;

    cout << "Insert Bench Finish. use time: " << insert_time << "s" << endl;
    cout << "Insert Bench Result: " << record_num / insert_time
            << " per second." << endl;

    return;
}

void Bench_Get_Seq(KvdbDS *db, int record_num, vector<string> &key_list) {
    cout << "Get Sequential Bench Start, record_num = " << record_num
            << ", Please wait ..." << endl;

    string value = string(TEST_BS, 'v');
    int key_len = KEY_LEN;

    KVTime tv_start;
    Status s;

    for (vector<string>::iterator iter = key_list.begin(); iter
            != key_list.end(); iter++) {
        string key = *iter;
        string get_data;
        //if ( key == "40999kkkkk")
        //{
        //    cout << "hello" << endl;
        //}
        s = db->Get(key.c_str(), key_len, get_data);
        if (!s.ok()) {
            cout << "Get key=" << key << " from DB failed" << endl;
        }
        if (strcmp(get_data.c_str(), value.c_str()) != 0) {
            cout << "Get key=" << key << "Inconsistent! " << endl;
            cout << "Value size = " << get_data.length() << endl;
            cout << "value = " << get_data << endl;
        }
    }

    KVTime tv_end;
    double get_time = (tv_end - tv_start) / 1000000.0;

    cout << "Get Sequential Bench Finish. use time: " << get_time << "s"
            << endl;
    cout << "Get Sequential Bench Result: " << record_num / get_time
            << " per second." << endl;

    return;
}

int Parse_Option(int argc, char** argv, string &filename, int &db_size,
                 int &record_num) {

    if (argc != 7) {
        return -1;
    }

    string str_f = "-f";
    string str_s = "-s";
    string str_n = "-n";
    if (strcmp(argv[1], str_f.c_str()) != 0 || strcmp(argv[3], str_s.c_str())
            != 0 || strcmp(argv[5], str_n.c_str()) != 0) {
        return -1;
    }

    filename = argv[2];
    db_size = atoi(argv[4]);
    record_num = atoi(argv[6]);

    if (db_size < 0 || record_num < 0) {
        return -1;
    }

    return 0;
}

void Bench(string file_path, int db_size, int record_num) {

    vector < string > key_list;
    Create_Keys(record_num, key_list);
    if (Create_DB(file_path, db_size) < 0) {
        return;
    }

    Options opts;
    KvdbDS *db = KvdbDS::Open_KvdbDS(file_path.c_str(), opts);

    for (int i = 0; i < 10; i++) {
        Bench_Insert(db, record_num, key_list);
        //db->Do_GC();
    }
    Bench_Get_Seq(db, record_num, key_list);

    delete db;
}

int main(int argc, char** argv) {
    string file_path;
    int db_size;
    int record_num;

    if (Parse_Option(argc, argv, file_path, db_size, record_num) < 0) {
        usage();
        return -1;
    }

    Bench(file_path, db_size, record_num);

    return 0;
}
