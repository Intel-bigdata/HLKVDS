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
#include <algorithm>
#include "Kvdb_Impl.h"
#include "hyperds/Options.h"

#define SEG_UNIT_SIZE 1024
#define KEY_SIZE 10
#define VALUE_SIZE 4096

#define OVERWRITE_TIMES 10

using namespace std;
using namespace kvdb;

struct thread_arg {
    KvdbDS *db;
    int key_start;
    int key_end;
    vector<string> *key_list;
    string* data;
    uint64_t *latency;
};

enum Benchmark_Type {
    WRITE,
    OVERWRITE,
    READ
};

struct benchmark_arg {
    string file_path;
    int db_size;
    int record_num;
    int thread_num;
    int segment_K;
    Benchmark_Type bench_type;
};

struct Lat_Stats {
    uint64_t med;
    uint64_t P_50;
    uint64_t P_75;
    uint64_t P_99;
    uint64_t P_999;
    uint64_t P_9999;
};

class LatMgr {
public:
    ~LatMgr() {
        lat_vec.clear();
    }

    void Push_back(uint64_t lat) {
        lat_vec.push_back(lat);
    }

    void Push_back_batch(uint64_t* lat_lst, int len) {
        for (int i=0; i < len; i++) {
            lat_vec.push_back(lat_lst[i]);
            //cout << "latency is :" << lat_lst[i] << endl;
        }
    }

    uint64_t GetAvg() {
        uint64_t total_lats = 0;
        for (vector<uint64_t>::iterator iter = lat_vec.begin(); iter != lat_vec.end(); iter++) {
            total_lats += *iter;
        }
        uint64_t avg = total_lats / (uint64_t)lat_vec.size();
        //cout << "total_lats = " << total_lats << ", size = " << lat_vec.size() << endl;
        return avg;
    }

    uint64_t GetMax() {
        return *max_element(lat_vec.begin(), lat_vec.end());
    }

    uint64_t GetMin() {
        return *min_element(lat_vec.begin(), lat_vec.end());
    }

    void GetStatistics(Lat_Stats &lat_stats) {
        sort(lat_vec.begin(), lat_vec.end());
        //for (vector<uint64_t>::iterator iter=lat_vec.begin(); iter!= lat_vec.end(); ++iter)
        //{
        //    cout << "sorted latency is: " << *iter << endl;
        //}
        int len = lat_vec.size();
        int med_idx = len / 2;
        int P_50_idx = (int) (len * 0.5) - 1;
        int P_75_idx = (int) (len * 0.75) - 1;
        int P_99_idx = (int) (len * 0.99) - 1;
        int P_999_idx = (int) (len * 0.999) - 1;
        int P_9999_idx = (int) (len * 0.9999) - 1;

        lat_stats.med    = lat_vec[med_idx];
        lat_stats.P_50   = lat_vec[P_50_idx];
        lat_stats.P_75   = lat_vec[P_75_idx];
        lat_stats.P_99   = lat_vec[P_99_idx];
        lat_stats.P_999  = lat_vec[P_999_idx];
        lat_stats.P_9999 = lat_vec[P_9999_idx];
    }

private:
    vector<uint64_t> lat_vec;
};

void usage() {
    cout << "Usage: ./Benchmark write|overwrite|read -f dbfile -s db_size \
-n num_records -t thread_num -seg segment_size(KB)" << endl;
}

int Create_DB(string filename, int db_size, int segment_K) {
    cout << "Start CreateDB, Please wait ..." << endl;
    int ht_size = db_size ;
    int segment_size = SEG_UNIT_SIZE * segment_K;

    Options opts;
    opts.hashtable_size = ht_size;
    opts.segment_size = segment_size;

    KVTime tv_start;
    KvdbDS *db = KvdbDS::Create_KvdbDS(filename.c_str(), opts);

    KVTime tv_end;
    double diff_time = (tv_end - tv_start) / 1000000.0;

    cout << "Create DB use time: " << diff_time << "s" << endl;
    if (!db) {
        return -1;
    }
    delete db;
    return 0;
}

KvdbDS* Open_DB(string filename) {
    cout << "Start OpenDB, Please wait ..." << endl;
    Options opts;
    KVTime tv_start;
    KvdbDS *db = KvdbDS::Open_KvdbDS(filename.c_str(), opts);
    KVTime tv_end;
    double diff_time = (tv_end - tv_start) / 1000000.0;
    cout << "Open DB use time: " << diff_time << "s" << endl;
    return db;
}

void Create_Keys(int record_num, vector<string> &key_list) {
    for (int index = 0; index < record_num; index++) {
        char c_key[KEY_SIZE+1] = "kkkkkkkkkk";
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
    vector<string> &key_list = *t_args->key_list;
    uint64_t *latency = t_args->latency;

    string *value = t_args->data;

    int value_size = VALUE_SIZE;
    int key_len = KEY_SIZE;
    Status s;

    for (int i = key_start; i < key_end + 1; i++) {
        string key = key_list[i];
        KVTime tv_start;
        s = db->Insert(key.c_str(), key_len, value->c_str(), value_size);
        KVTime tv_end;
        latency[i - key_start] = (uint64_t)(tv_end - tv_start);
        if (!s.ok()) {
            cout << "Insert key=" << key << "to DB failed!" << endl;
        }
    }
    return NULL;

}

double Bench_Insert(KvdbDS *db, int record_num, vector<string> &key_list,
                    int thread_num, LatMgr *total_lat_mgr = NULL) {
    cout << "Start Benchmark Test: Insert , record_num = " << record_num << ", Please wait ..." << endl;

    string data = string(VALUE_SIZE, 'v');

    thread_arg arglist[thread_num];
    pthread_t pidlist[thread_num];
    for (int i = 0; i < thread_num; i++) {
        int start = (record_num / thread_num) * i;
        int end = (record_num / thread_num) * (i + 1) - 1;
        arglist[i].db = db;
        arglist[i].key_start = start;
        arglist[i].key_end = end;
        arglist[i].key_list = &key_list;
        arglist[i].data = &data;
        arglist[i].latency = new uint64_t[end - start + 1];
    }

    KVTime tv_start;
    for (int i = 0; i < thread_num; i++) {
        pthread_create(&pidlist[i], NULL, fun_insert, &arglist[i]);
    }
    //db->printDbStates();
    for (int i=0; i<thread_num; i++) {
        pthread_join(pidlist[i], NULL);
    }

    KVTime tv_end;
    double diff_time = (tv_end - tv_start) / 1000000.0;

    LatMgr lat_mgr;
    for (int i = 0; i < thread_num; i++) {
        lat_mgr.Push_back_batch(arglist[i].latency, record_num/thread_num);

        //if overwrite, save to total_lat_mgr
        if (total_lat_mgr) {
            total_lat_mgr->Push_back_batch(arglist[i].latency, record_num/thread_num);
        }

        delete[] arglist[i].latency;
    }

    Lat_Stats lat_stats;
    lat_mgr.GetStatistics(lat_stats);

    double iops = record_num / diff_time;

    uint64_t total_size = (uint64_t)record_num * (KEY_SIZE + VALUE_SIZE);
    double throughput = total_size / (diff_time * 1024 * 1024);

    uint64_t avg = lat_mgr.GetAvg();
    uint64_t min = lat_mgr.GetMin();
    uint64_t max = lat_mgr.GetMax();

    cout << "Insert Bench Finish. Total time: " << diff_time << "s" <<endl;
    cout << "Performance Report     :   IOPS = " <<  iops << ", Average Latency = "
            << avg << "us, Throughput = " << throughput << "MB/s." << endl;
    cout << "Latency Report(usec)   :   Min = " << min << ", Max = "
            << max << ", Med = " << lat_stats.med << "." << endl;
    cout << "Latency Statistic(usec):   P50 = " << lat_stats.P_50 << ", P75 = "
            << lat_stats.P_75 << ", P99 = " << lat_stats.P_99 << ", P999 = "
            << lat_stats.P_999 << ", P9999 = " << lat_stats.P_9999 << endl;

    return diff_time;
}

void* fun_get(void *arg) {
    thread_arg *t_args = (thread_arg*) arg;
    KvdbDS *db = t_args->db;
    int key_start = t_args->key_start;
    int key_end = t_args->key_end;
    vector<string> &key_list = *t_args->key_list;
    uint64_t *latency = t_args->latency;

    string *value = t_args->data;
    int key_len = KEY_SIZE;
    Status s;

    for (int i = key_start; i < key_end + 1; i++) {
        string key = key_list[i];
        string get_data;
        KVTime tv_start;
        s = db->Get(key.c_str() ,key_len, get_data);
        KVTime tv_end;
        latency[i - key_start] = (uint64_t)(tv_end - tv_start);

        if (!s.ok()) {
            cout << "Get key = " << key << "from DB failed!" << endl;
        }
        if (strcmp(get_data.c_str(), value->c_str()) != 0) {
            cout << "Get key=" << key << "Inconsistent! " << endl;
            cout << "Value size = " << get_data.length() << endl;
            cout << "value = " << get_data << endl;
        }
    }
    return NULL;
}

void Bench_Get_Seq(KvdbDS *db, int record_num, vector<string> &key_list,
                    int thread_num) {
    cout << "Start Benchmark Test: Get , record_num = " << record_num << ", Please wait ..." << endl;

    string data = string(VALUE_SIZE, 'v');

    thread_arg arglist[thread_num];
    pthread_t pidlist[thread_num];
    for (int i = 0; i < thread_num; i++) {
        int start = (record_num / thread_num) * i;
        int end = (record_num / thread_num) * (i + 1) - 1;
        arglist[i].db = db;
        arglist[i].key_start = start;
        arglist[i].key_end = end;
        arglist[i].key_list = &key_list;
        arglist[i].data = &data;
        arglist[i].latency = new uint64_t[end - start + 1];
    }

    KVTime tv_start;
    for (int i = 0; i < thread_num; i++) {
        pthread_create(&pidlist[i], NULL, fun_get, &arglist[i]);
    }
    //db->printDbStates();
    for (int i=0; i<thread_num; i++) {
        pthread_join(pidlist[i], NULL);
    }

    KVTime tv_end;
    double diff_time = (tv_end - tv_start) / 1000000.0;

    LatMgr lat_mgr;
    for (int i = 0; i < thread_num; i++) {
        lat_mgr.Push_back_batch(arglist[i].latency, record_num/thread_num);
        delete[] arglist[i].latency;
    }

    Lat_Stats lat_stats;
    lat_mgr.GetStatistics(lat_stats);

    double iops = record_num / diff_time;

    uint64_t total_size = (uint64_t)record_num * (KEY_SIZE + VALUE_SIZE);
    double throughput = total_size / (diff_time * 1024 * 1024);

    uint64_t avg = lat_mgr.GetAvg();
    uint64_t min = lat_mgr.GetMin();
    uint64_t max = lat_mgr.GetMax();

    cout << "Get Bench Finish. Total time: " << diff_time << "s" <<endl;
    cout << "Performance Report     :   IOPS = " <<  iops << ", Average Latency = "
            << avg << "us, Throughput = " << throughput << "MB/s." << endl;
    cout << "Latency Report(usec)   :   Min = " << min << ", Max = "
            << max << ", Med = " << lat_stats.med << "." << endl;
    cout << "Latency Statistic(usec):   P50 = " << lat_stats.P_50 << ", P75 = "
            << lat_stats.P_75 << ", P99 = " << lat_stats.P_99 << ", P999 = "
            << lat_stats.P_999 << ", P9999 = " << lat_stats.P_9999 << endl;

    return;
}

int Parse_Option(int argc, char** argv, benchmark_arg &bm_arg) {
    if (argc != 12) {
        cout << "Please Input all the parameters!" << endl;
        return -1;
    }

    if (!strcmp(argv[1], "write")) {
        bm_arg.bench_type = Benchmark_Type::WRITE;
    }
    else if (!strcmp(argv[1], "overwrite")) {
        bm_arg.bench_type = Benchmark_Type::OVERWRITE;
    }
    else if (!strcmp(argv[1], "read")) {
        bm_arg.bench_type = Benchmark_Type::READ;
    }
    else {
        cout << "Please Input Correct benchmarks !" << endl;
        return -1;
    }

    string str_f = "-f";
    string str_s = "-s";
    string str_n = "-n";
    string str_t = "-t";
    string str_seg = "-seg";

    if (strcmp(argv[2], str_f.c_str()) !=0 || \
        strcmp(argv[4], str_s.c_str()) != 0 || \
        strcmp(argv[6], str_n.c_str()) != 0 || \
        strcmp(argv[8], str_t.c_str()) != 0 || \
        strcmp(argv[10], str_seg.c_str()) != 0) {
        cout << "Please Input Correct parameter!" << endl;
        return -1;
    }

    bm_arg.file_path = argv[3];
    bm_arg.db_size = atoi(argv[5]);
    bm_arg.record_num = atoi(argv[7]);
    bm_arg.thread_num = atoi(argv[9]);
    bm_arg.segment_K = atoi(argv[11]);
    
    if (bm_arg.db_size < 0 ||  bm_arg.record_num < 0 || \
        bm_arg.thread_num < 0 || bm_arg.segment_K < 0) {
        cout << "Parametes error, Please Input Positive Integer!" << endl;
        return -1;
    }

    return 0;
}

void Bench_Write(benchmark_arg bm_arg) {
    string file_path = bm_arg.file_path;
    int db_size = bm_arg.db_size;
    int record_num =bm_arg.record_num;
    int thread_num = bm_arg.thread_num;
    int segment_K = bm_arg.segment_K;

    vector<string> key_list;
    Create_Keys(record_num, key_list);
    if (Create_DB(file_path, db_size, segment_K) < 0) {
        cout << "Create DB Fail!!!" <<endl;
        return;
    }

    KvdbDS *db = Open_DB(file_path);

    Bench_Insert(db, record_num, key_list, thread_num);
    delete db;
}

void Bench_Overwrite(benchmark_arg bm_arg) {
    string file_path = bm_arg.file_path;
    int db_size = bm_arg.db_size;
    int record_num =bm_arg.record_num;
    int thread_num = bm_arg.thread_num;
    int segment_K = bm_arg.segment_K;

    vector<string> key_list;
    Create_Keys(record_num, key_list);
    if (Create_DB(file_path, db_size, segment_K) < 0) {
        cout << "Create DB Fail!!!" <<endl;
        return;
    }

    KvdbDS *db = Open_DB(file_path);

    double total_time;
    LatMgr *total_lat_mgr = new LatMgr;
    for (int i = 0; i < OVERWRITE_TIMES; i++) {
        cout << "======================";
        cout << "Insert " << i << " Start!!!";
        cout << "======================" << endl;

        total_time += Bench_Insert(db, record_num, key_list, thread_num, total_lat_mgr);

        cout << "======================";
        cout << "Insert " << i << " Completed!!!";
        cout << "======================" << endl;
    }
    cout << "======================";
    cout << "Finsh All Insert!!!";
    cout << "======================" << endl;

    Lat_Stats lat_stats;
    total_lat_mgr->GetStatistics(lat_stats);

    uint64_t total_record_num = record_num * OVERWRITE_TIMES;
    double iops = total_record_num / total_time;

    uint64_t total_size = (uint64_t)total_record_num * (KEY_SIZE + VALUE_SIZE);
    double throughput = total_size / (total_time * 1024 * 1024);

    uint64_t avg = total_lat_mgr->GetAvg();
    uint64_t min = total_lat_mgr->GetMin();
    uint64_t max = total_lat_mgr->GetMax();

    cout << "Overwrite Bench Finish. Total time: " << total_time << "s" <<endl;
    cout << "Performance Report     :   IOPS = " <<  iops << ", Average Latency = "
            << avg << "us, Throughput = " << throughput << "MB/s." << endl;
    cout << "Latency Report(usec)   :   Min = " << min << ", Max = "
            << max << ", Med = " << lat_stats.med << "." << endl;
    cout << "Latency Statistic(usec):   P50 = " << lat_stats.P_50 << ", P75 = "
            << lat_stats.P_75 << ", P99 = " << lat_stats.P_99 << ", P999 = "
            << lat_stats.P_999 << ", P9999 = " << lat_stats.P_9999 << endl;

    delete total_lat_mgr;
    delete db;
}

void Bench_Read(benchmark_arg bm_arg)
{
    string file_path = bm_arg.file_path;
    int record_num =bm_arg.record_num;
    int thread_num = bm_arg.thread_num;

    vector<string> key_list;
    Create_Keys(record_num, key_list);

    KvdbDS *db = Open_DB(file_path);
    db->ClearReadCache();
    Bench_Get_Seq(db, record_num, key_list, thread_num);
    delete db;
}

int main(int argc, char** argv) {

    benchmark_arg bm_arg;

    if(Parse_Option(argc, argv, bm_arg) < 0) {
        usage();
        return -1;
    }

    switch(bm_arg.bench_type) {
        case WRITE:
            Bench_Write(bm_arg);
            break;
        case OVERWRITE:
            Bench_Overwrite(bm_arg);
            break;
        case READ:
            Bench_Read(bm_arg);
            break;
        default:
            break;
    }
    return 0;
}
