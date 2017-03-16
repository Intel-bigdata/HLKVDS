//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef DB_BENCH_DB_BENCH__HPP
#define DB_BENCH_DB_BENCH__HPP
#include <stdio.h>                                                                 
#include <unistd.h>                                                                
#include <getopt.h>                                                                
#include <stdlib.h>
#include <vector>
#include <string>

#include "slice.h"
#include "mutexlock.h"
#include "testutil.h"
#include "histogram.h"
#include "my_kv.h"
#include "env.h"

namespace general_db_bench {
/////////////////////////////////the FLAGS **ONLY a few is used now.**
// Comma-separated list of operations to run in the specified order
//   Actual benchmarks:
//      fillseq       -- write N values in sequential key order in async mode
//      fillrandom    -- write N values in random key order in async mode
//      overwrite     -- overwrite N values in random key order in async mode
//      fillsync      -- write N/100 values in random key order in sync mode
//      fill100K      -- write N/1000 100K values in random order in async mode
//      deleteseq     -- delete N keys in sequential order
//      deleterandom  -- delete N keys in random order
//      readseq       -- read N times sequentially
//      readreverse   -- read N times in reverse order
//      readrandom    -- read N times in random order
//      readmissing   -- read N missing keys in random order
//      readhot       -- read N times in random order from 1% section of DB
//      seekrandom    -- N random seeks
//      crc32c        -- repeated crc32c of 4K of data
//      acquireload   -- load N*1000 times
//   Meta operations:
//      compact     -- Compact the entire DB
//      stats       -- Print DB stats
//      sstables    -- Print sstable info
//      heapprofile -- Dump a heap profile (if supported by this port)
static const char* FLAGS_benchmarks = "fillseq,"
    "fillrandom,"
    "readrandom,"
    "readseq,"
/*		"fillsync,"
 "overwrite,"
 "readrandom,"  // Extra run to allow previous compactions to quiesce
 "readseq,"
 "readreverse,"
 "compact,"
 "readrandom,"
 "readreverse,"
 "fill100K,"
 "crc32c,"
 "snappycomp,"
 "snappyuncomp,"
 "acquireload,"   */
;

// Number of key/values to place in database
static int FLAGS_num = 1000000;

// Number of concurrent threads to run.
static int FLAGS_threads = 1;

// Size of each value
static int FLAGS_value_size = 100;

// Size of each key
static int FLAGS_key_size = 16;

// Print histogram of operation timings
static bool FLAGS_histogram = false;

// If true, do not destroy the existing database.  If you set this
// flag and also specify a benchmark that wants a fresh database, that
// benchmark will fail.
static bool FLAGS_use_existing_db = false;

// Use the db with the following name.
static const char* FLAGS_db = "/tmp/dbbench";

// Not support:
// Ratio of read operation
static float FLAGS_readratio = 0;

// Arrange to generate values that shrink to this fraction of
// their original size after compression
static double FLAGS_compression_ratio = 0.5;

// Number of bytes to buffer in memtable before compacting
// (initialized to default value by "main")
static int FLAGS_write_buffer_size = 0;

// Number of bytes to use as a cache of uncompressed data.
// Negative means use default settings.
static int FLAGS_cache_size = -1;

// Maximum number of files to keep open at the same time (use default if == 0)
static int FLAGS_open_files = 0;

// Bloom filter bits per key.
// Negative means use default settings.
static int FLAGS_bloom_bits = -1;
///////////////////////////

struct benchoptions {
    const char* benchmark;
    const char* db_name;
    int num;
    float read_ratio;
    int threads_num;
    bool histograming;
    bool use_exsisting;
    int value_size;
    int key_size;
    benchoptions() :
        benchmark(FLAGS_benchmarks), db_name(FLAGS_db), num(FLAGS_num),
                read_ratio(FLAGS_readratio), histograming(FLAGS_histogram),
                threads_num(FLAGS_threads),
                use_exsisting(FLAGS_use_existing_db),
                value_size(FLAGS_value_size), key_size(FLAGS_key_size) {
    }
};
void help(const char* biname) {
    printf("Usage: %s [options]\n"
        "Options:\n"
        " -b, --benchmark=[fillseq|fillrandom|readseq|readrandom] ...\n"
        " -d, --db_name=<name>                 name of the database.\n"
        " --histogram,                         use histogram to record statstics.\n"
        " -n, --num=<number_of_ops>            operation number.\n"
        " -r, --read_ratio=<read_ratio>        n/a. read ratio of operations.\n"
        " -t, --threads_num=<thread_num>       thread to run.\n"
        " -u, --use_exsisting                  n/a.\n"
        " -v, --value_size=<size>              value size in bytes.\n"
        " -k, --key_size=<size>                key size in bytes.\n"
        " -h, --help                           show this message.\n", biname);
    exit(-1);
}
void parse_args(int argc, char*argv[], struct benchoptions *ret) {
    struct option long_options[] = { { "benchmark", optional_argument, NULL,
            'b' }, { "db_name", required_argument, NULL, 'd' },
            { "histogram", no_argument, NULL, 'z' + 1 },//only long format supported
            { "help", no_argument, NULL, 'h' }, { "num", required_argument,
                    NULL, 'n' },
            { "read_ratio", required_argument, NULL, 'r' }, { "threads_num",
                    required_argument, NULL, 't' }, { "use_exsisting",
                    no_argument, NULL, 'u' }, { "value_size",
                    required_argument, NULL, 'v' }, { "key_size",
                    required_argument, NULL, 'k' }, { 0, 0, 0, 0 } };
    int opt_index, c;
    while (1) {
        c = getopt_long(argc, argv, "b:d:hn:r:t:uv:", long_options, &opt_index);
        if (-1 == c)
            break;
        switch (c) {
            case 'b':
                if (ret)
                    ret->benchmark = optarg;//if null, all bench model will be run.
                else
                    FLAGS_benchmarks = optarg;
                break;
            case 'd':
                if (ret)
                    ret->db_name = optarg;
                else
                    FLAGS_db = optarg;
                break;
            case ('z' + 1):
                if (ret)
                    ret->histograming = true;
                else
                    FLAGS_histogram = true;
                break;
            case 'n':
                if (ret)
                    ret->num = atoi(optarg);
                else
                    FLAGS_num = atoi(optarg);
                break;
            case 'r':
                if (ret)
                    ret->read_ratio = atof(optarg);
                else
                    FLAGS_readratio = atof(optarg);
                break;
            case 't':
                if (ret)
                    ret->threads_num = atoi(optarg);
                else
                    FLAGS_threads = atoi(optarg);
                break;
            case 'u':
                if (ret)
                    ret->use_exsisting = true;
                else
                    FLAGS_use_existing_db = true;
                break;
            case 'v':
                if (ret)
                    ret->value_size = atoi(optarg);
                else
                    FLAGS_value_size = atoi(optarg);
                break;
            case 'k':
                if (ret)
                    ret->key_size = atoi(optarg);
                else
                    FLAGS_key_size = atoi(optarg);
                break;
            case 'h':
            case '?':
            default:
                help(argv[0]);
        };
    }
}//void parse_args(int argc, char*argv[], struct benchoptions *ret)
namespace {
static Slice TrimSpace(Slice s) {
    int start = 0;
    while (start < s.size() && isspace(s[start])) {
        start++;
    }
    int limit = s.size();
    while (limit > start && isspace(s[limit - 1])) {
        limit--;
    }
    return Slice(s.data() + start, limit - start);
}

static void AppendWithSpace(std::string* str, Slice msg) {
    if (msg.empty())
        return;
    if (!str->empty()) {
        str->push_back(' ');
    }
    str->append(msg.data(), msg.size());
}

class Stats {
private:
    double start_;
    double finish_;
    double seconds_;
    int done_;
    int next_report_;
    int64_t bytes_;
    double last_op_finish_;
    Histogram hist_;
    bool histograming_;
    std::string message_;
public:
    Stats(bool hist) :
        histograming_(hist) {
        Start();
    }

    void Start() {
        next_report_ = 100;
        last_op_finish_ = start_;
        hist_.Clear();
        done_ = 0;
        bytes_ = 0;
        seconds_ = 0;
        start_ = Env::Default()->NowMicros();
        finish_ = start_;
        message_.clear();
    }

    void Merge(const Stats& other) {
        hist_.Merge(other.hist_);
        done_ += other.done_;
        bytes_ += other.bytes_;
        seconds_ += other.seconds_;
        if (other.start_ < start_)
            start_ = other.start_;
        if (other.finish_ > finish_)
            finish_ = other.finish_;
        // Just keep the messages from one thread
        if (message_.empty())
            message_ = other.message_;
    }
    void Stop() {
        finish_ = Env::Default()->NowMicros();
        seconds_ = (finish_ - start_) * 1e-6;
    }
    void AddMessage(Slice msg) {
        AppendWithSpace(&message_, msg);
    }
    void FinishedSingleOp() {
        if (histograming_) {
            double now = Env::Default()->NowMicros();
            double micros = now - last_op_finish_;
            hist_.Add(micros);
            if (micros > 20000) {
                fprintf(stderr, "long op: %.1f micros%30s\r", micros, "");
                fflush( stderr);
            }
            last_op_finish_ = now;
        }

        done_++;
        if (done_ >= next_report_) {
            if (next_report_ < 1000)
                next_report_ += 100;
            else if (next_report_ < 5000)
                next_report_ += 500;
            else if (next_report_ < 10000)
                next_report_ += 1000;
            else if (next_report_ < 50000)
                next_report_ += 5000;
            else if (next_report_ < 100000)
                next_report_ += 10000;
            else if (next_report_ < 500000)
                next_report_ += 50000;
            else
                next_report_ += 100000;
            fprintf(stderr, "... finished %d ops%30s\r", done_, "");
            fflush( stderr);
        }
    }
    void AddBytes(int64_t n) {
        bytes_ += n;
    }

    void Report(const Slice& name) {
        // Pretend at least one op was done in case we are running a benchmark
        // that does not call FinishedSingleOp().
        if (done_ < 1)
            done_ = 1;
        std::string extra;
        if (bytes_ > 0) {
            // Rate is computed on actual elapsed time, not the sum of per-thread
            // elapsed times.
            double elapsed = (finish_ - start_) * 1e-6;
            char rate[100];
            snprintf(rate, sizeof(rate), "%6.1f MB/s",
                     (bytes_ / 1048576.0) / elapsed);
            extra = rate;
        }
        AppendWithSpace(&extra, message_);

        fprintf(stdout, "%-12s : %11.3f micros/op;%s%s\n",
                name.ToString().c_str(), seconds_ * 1e6 / done_,
                (extra.empty() ? "" : " "), extra.c_str());
        if (histograming_) {
            fprintf(stdout, "Microseconds per op:\n%s\n",
                    hist_.ToString().c_str());
        }
        fflush( stdout);
    }
};//class Stats

// State shared by all concurrent executions of the same benchmark.
struct SharedState {
    port::Mutex mu;
    port::CondVar cv;
    int total;

    // Each thread goes through the following states:
    //    (1) initializing
    //    (2) waiting for others to be initialized
    //    (3) running
    //    (4) done

    int num_initialized;
    int num_done;
    bool start;
    SharedState() :
        cv(&mu) {
    }
};
class SimpleRandom {
public:
    SimpleRandom(int sd) :
        data_(""), pos_(0) {
        int mask = 2147483647;
        char buf[64];
        while (data_.size() < 10000000) {//approximately 9 Megabytes
            mask = (mask + sd) << sd;
            sd += sd << 5;
            sd += 0xabcd;
            memset(buf, 0, sizeof(buf));
            int v = mask >> 8;
            mask |= v;
            mask |= v << 8;
            mask |= v << 16;
            mask |= v << 24;
            snprintf(buf, sizeof(buf), "%x", mask);
            data_.append(buf);
        }
    }
    const char* Next(int len) {
        if (pos_ > data_.size() - len) {
            pos_ = 0;
        }
        const char* ret = data_.substr(pos_, len).c_str();
        pos_ += len;
        last_len_ = len;
        return ret;
    }
    const char* Prev() {
        return data_.substr(pos_ - last_len_, last_len_).c_str();
    }
private:
    std::string data_;
    int last_len_;
    int pos_;
};//class SimpleRandom

// Per-thread state for concurrent executions of the same benchmark.
struct ThreadState {
    int tid; // 0..n-1 when running in n threads
    SimpleRandom sr; // use a simple random generator
    Stats stats;
    SharedState* shared;

    ThreadState(int index, bool hist) :
        tid(index), sr(index + 0x1234), stats(hist) {
    }
};
} // namespace


class Benchmark {
private:
    struct benchoptions opts_;
    my_kv_interface *kv_itf_;
    std::vector<const char*> random_keylist_;
    void PrintWarnings() {
#if defined(__GNUC__) && !defined(__OPTIMIZE__)
        fprintf(stdout,
                "WARNING: Optimization is disabled: benchmarks unnecessarily slow\n"
        );
#endif
#ifndef NDEBUG
        fprintf(stdout,
                "WARNING: Assertions are enabled; benchmarks unnecessarily slow\n");
#endif
        // ***Don't include snappy_test now.***
    }
    void PrintEnvironment() {
#if defined(__linux)
        time_t now = time(NULL);
        fprintf(stderr, "Date:       %s", ctime(&now)); // ctime() adds newline

        FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
        if (cpuinfo != NULL) {
            char line[1000];
            int num_cpus = 0;
            std::string cpu_type;
            std::string cache_size;
            while (fgets(line, sizeof(line), cpuinfo) != NULL) {
                const char* sep = strchr(line, ':');
                if (sep == NULL) {
                    continue;
                }
                Slice key = TrimSpace(Slice(line, sep - 1 - line));
                Slice val = TrimSpace(Slice(sep + 1));
                if (key == "model name") {
                    ++num_cpus;
                    cpu_type = val.ToString();
                } else if (key == "cache size") {
                    cache_size = val.ToString();
                }
            }
            fclose(cpuinfo);
            fprintf(stderr, "CPU:        %d * %s\n", num_cpus, cpu_type.c_str());
            fprintf(stderr, "CPUCache:   %s\n", cache_size.c_str());
        }
#endif
    }//void PrintEnvironment()

    void PrintHeader() {
        PrintEnvironment();
        fprintf(stdout, "Keys:       %d bytes each\n", opts_.key_size);
        fprintf(stdout, "Values:     %d bytes each\n", opts_.value_size);
        fprintf(stdout, "Entries:    %d\n", opts_.num);
        fprintf(stdout, "FileSize:   %.1f MB (estimated)\n",
                (opts_.key_size + opts_.value_size) * opts_.num / 1048576.0);
        PrintWarnings();
        fprintf(stdout, "------------------------------------------------\n");
    }

public:
    Benchmark(int argc, char*argv[], my_kv_interface* kv_itf) :
        kv_itf_(kv_itf) {
        parse_args(argc, argv, &opts_);
    }
    ~Benchmark() {
        Release_prestore_keylist();
    }

    void Run() {
        PrintHeader();

        //parse benchmark_tags
        const char* ptr = opts_.benchmark;
        const char* name = ptr;
        void (Benchmark::*method)(ThreadState*) = NULL;
        int count = 0;

        while (1) {
            if ('|' == *ptr || ',' == *ptr || '\0' == *ptr) {
                BenchType bt = Add_bench(name, ptr - name, count, method);
                if (method != NULL) {
                    if (!opts_.use_exsisting) {
                        kv_itf_->destroy(opts_.db_name);
                    }
                    kv_itf_->open(opts_.db_name);
                    if (bt < ReadBench && !opts_.use_exsisting) {
                        Initialize_db_for_read(bt == ReadSeqBench);
                    }
                    //run
                    RunBenchmark(opts_.threads_num, Slice(name, ptr - name),
                                 method);//start multi-threads test
                    kv_itf_->close();
                }
                if ('\0' == *ptr)
                    break;
                name = ptr + 1;
            }
            ptr++;
        }//while(1)
    }//Run()
private:
    struct ThreadArg {
        Benchmark* bm;
        SharedState* shared;
        ThreadState* thread;
        void (Benchmark::*method)(ThreadState*);
    };
    static void ThreadBody(void* v) {
        ThreadArg* arg = reinterpret_cast<ThreadArg*> (v);
        SharedState* shared = arg->shared;
        ThreadState* thread = arg->thread;
        {
            MutexLock l(&shared->mu);
            shared->num_initialized++;
            printf("[DBG] thread %d initialized.\n", thread->tid);
            if (shared->num_initialized >= shared->total) {
                shared->cv.SignalAll();
            }
            while (!shared->start) {
                shared->cv.Wait();//here will unlock the mutex && sleep
            }
        }

        thread->stats.Start();
        (arg->bm->*(arg->method))(thread);
        thread->stats.Stop();

        {
            MutexLock l(&shared->mu);
            shared->num_done++;
            printf("[DBG] thread %d done.\n", thread->tid);
            if (shared->num_done >= shared->total) {
                shared->cv.SignalAll();
            }
        }
    }//static void ThreadBody(void* v)

    void RunBenchmark(int n, Slice name, void(Benchmark::*method)(ThreadState*)) {
        SharedState shared;
        shared.total = n;//thread's number
        shared.num_initialized = 0;
        shared.num_done = 0;
        shared.start = false;

        ThreadArg* arg = new ThreadArg[n];
        for (int i = 0; i < n; i++) {
            arg[i].bm = this;
            arg[i].method = method;
            arg[i].shared = &shared;
            arg[i].thread = new ThreadState(i, opts_.histograming);//generate_random_here
            arg[i].thread->shared = &shared;
            Env::Default()->StartThread(ThreadBody, &arg[i]);
        }

        shared.mu.Lock();
        while (shared.num_initialized < n) {
            shared.cv.Wait();
        }

        shared.start = true;//threads will start to run test
        shared.cv.SignalAll();
        while (shared.num_done < n) {
            shared.cv.Wait();
        }
        shared.mu.Unlock();

        for (int i = 1; i < n; i++) {
            arg[0].thread->stats.Merge(arg[i].thread->stats);
        }
        arg[0].thread->stats.Report(name);

        for (int i = 0; i < n; i++) {
            delete arg[i].thread;
        }
        delete[] arg;
    }//RunBenchmark (int n, Slice name, void (Benchmark::*method)(ThreadState*))
    enum BenchType {
        ReadRandomBench,
        ReadSeqBench,
        ReadBench,
        WriteBench,
        ErrorBench
    };
    BenchType Add_bench(const char* name, int len, int &count,
                        void(Benchmark::*(&method))(ThreadState*)) {
        Slice benchname(name, len);
        BenchType ret = WriteBench;

        if (benchname == Slice("fillseq")) {
            printf("[DBG] fillseq selected!\n");
            method = &Benchmark::WriteSeq;
        } else if (benchname == Slice("fillrandom")) {
            printf("[DBG] fillrandom selected!\n");
            method = &Benchmark::WriteRandom;
        } else if (benchname == Slice("readseq")) {
            printf("[DBG] readseq selected!\n");
            method = &Benchmark::ReadSeq;
            ret = ReadSeqBench;
        } else if (benchname == Slice("readrandom")) {
            printf("[DBG] readrandom selected!\n");
            method = &Benchmark::ReadRandom;
            ret = ReadRandomBench;
        } else {
            method = NULL;
            if (benchname != Slice("")) {
                fprintf(stderr, "unknown benchmark '%s'\n",
                        benchname.ToString().c_str());
                ret = ErrorBench;
            }
        }
        if (ErrorBench != ret) {
            count++;
        }
        return ret;
    }//int Add_bench(const char* name, int len, int &count, void (Benchmark::*&method)(ThreadState*))

    void WriteSeq(ThreadState* thread) {
        DoWrite(thread, true);
    }

    void WriteRandom(ThreadState* thread) {
        DoWrite(thread, false);
    }

    void DoWrite(ThreadState* thread, bool seq) {
        int ret;
        int64_t bytes = 0;
        const int kKeySize = opts_.key_size;
        const int num = opts_.num;
        const int kValueSize = opts_.value_size;
        char* buf = new char[kKeySize + 1];
        std::string key(kKeySize, '0');

        for (int i = 0; i < num; i++) {
            if (seq) {
                const int k = i;
                snprintf(buf, kKeySize + 1, "%-d", k);
                key.replace(0, strlen(buf), buf);
            } else {
                const char* rkey = thread->sr.Next(kKeySize);
                key.replace(0, strlen(rkey), rkey);
            }
            ret = kv_itf_->put(key.c_str(), thread->sr.Next(kValueSize));//write something into db.
            bytes += kValueSize + key.size();
            if (0 != ret) { //TODO - err_sys such as DB_FULL
                fprintf(stderr, "Insert error - K:%s, V:%s\n", key.c_str(),
                        thread->sr.Prev());
                //exit(1); //continue here ?
            }
            thread->stats.FinishedSingleOp();//record some statstics for a single operation.
        }
        thread->stats.AddBytes(bytes);
        delete[] buf;
    }//DoWrite(ThreadState* thread, bool seq)

    void ReadRandom(ThreadState* thread) {
        DoRead(thread, false);
    }//ReadRandom(ThreadState* thread)

    void ReadSeq(ThreadState* thread) {
        DoRead(thread, true);
    }//ReadSeq(ThreadState* thread)

    void DoRead(ThreadState* thread, bool seq) {
        const int num = opts_.num;
        const int kKeySize = opts_.key_size;
        const int kValueSize = opts_.value_size;
        const char* key = NULL;
        char* buf = new char[kKeySize + 1];

        int found = 0;
        int64_t bytes = 0;

        for (int i = 0; i < num; i++) {
            std::string tmp(kKeySize, '0');
            if (seq) {
                snprintf(buf, kKeySize + 1, "%-d", i);
                tmp.replace(0, strlen(buf), buf);
                key = tmp.c_str();
            } else {
                key = random_keylist_[i];//operator[]?
            }
            const char* ret = kv_itf_->get(key);
            if (ret) {
                found++;
                bytes += kKeySize + kValueSize;
            }
            //	if(NULL == ret){
            //		fprintf(stderr, "Not found - K:%s\n", key);
            //	}
            thread->stats.FinishedSingleOp();
        }
        delete[] buf;
        char msg[100];
        snprintf(msg, sizeof(msg), "(%d of %d found)", found, num);
        thread->stats.AddMessage(msg);
        thread->stats.AddBytes(bytes);

    }//DoRead(ThreadState* thread, bool seq)

    void Initialize_db_for_read(bool seq) {
        printf("[DBG] Initialize_db_for_read...\n");
        const int num = opts_.num;
        const int kKeySize = opts_.key_size;
        const int kValueSize = opts_.value_size;
        SimpleRandom sr(0);//seed = 0

        if (!seq && random_keylist_.size() == 0) {//generate random keys. *is not thread safe*
            for (int i = 0; i < num; i++) {
                const int k = i;
                char *key = new char[kKeySize + 1];
                strncpy(key, sr.Next(kKeySize), kKeySize + 1);
                random_keylist_.push_back(key);
            }
        }

        //insert into db
        const char* key = NULL;
        char* buf = new char[kKeySize + 1];
        for (int i = 0; i < num; i++) {
            std::string tmp(kKeySize, '0');
            if (seq) {
                snprintf(buf, kKeySize + 1, "%-d", i);
                tmp.replace(0, strlen(buf), buf);
                key = tmp.c_str();
            } else {
                key = random_keylist_[i];
            }
            int ret = kv_itf_->put(key, sr.Next(kValueSize));
            if (0 != ret) { //TODO - err_sys such as DB_FULL
                fprintf(stderr, "Insert error - K:%s, V:%s\n", key, sr.Prev());
                exit(1); //continue here ?
            }
        }
        delete[] buf;
        printf("[DBG] Initialize_db_for_read Done.\n");
    }//void Initialize_db_for_read(bool seq)

    void Release_prestore_keylist() {
        for (std::vector<const char*>::iterator itr = random_keylist_.begin(); itr
                != random_keylist_.end(); itr++) {
            delete *itr;
        }
    }//void Release_prestore_keylist()

    //TODO - other test_cases add here
};//class Benchmark
}//namespace general_db_bench

#endif
