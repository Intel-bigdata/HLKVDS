//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef _KV_DB_UTILS_H_
#define _KV_DB_UTILS_H_

#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include "Db_Structure.h"

namespace kvdb {
class KVTime {
public:
    static inline size_t SizeOf() {
        return sizeof(time_t);
    }
    static const char* ToChar(KVTime& _time);
    static time_t GetNow();
    static const char* GetNowChar();

    void SetTime(time_t _time);
    time_t GetTime();
    timeval GetTimeval();
    void Update();

    KVTime();
    KVTime(const KVTime& toBeCopied);
    KVTime& operator=(const KVTime& toBeCopied);
    bool operator>(const KVTime& toBeCopied);
    bool operator<(const KVTime& toBeCopied);
    bool operator==(const KVTime& toBeCopied);
    int64_t operator-(const KVTime& toBeCopied);
    ~KVTime();

private:
    timeval tm_;

};

class Thread {
public:
    Thread();
    virtual ~Thread();

    int Start();
    int Join();
    int Detach();
    bool Is_started() const;
    bool Am_self() const;
    pthread_t Self();

    virtual void* Entry() = 0;

protected:
    pthread_t tid_;
    int running_;
    int detached_;

    static void* runThread(void* arg);
};

class Mutex {
public:
    Mutex() {
        pthread_mutex_init(&mtx_, NULL);
    }
    virtual ~Mutex() {
        pthread_mutex_destroy(&mtx_);
    }

    int Lock() {
        return pthread_mutex_lock(&mtx_);
    }
    int Trylock() {
        return pthread_mutex_trylock(&mtx_);
    }
    int Unlock() {
        return pthread_mutex_unlock(&mtx_);
    }

private:
    friend class Cond;
    pthread_mutex_t mtx_;
};

class Cond {
public:
    Cond(Mutex& mutex) :
        lock_(mutex) {
        pthread_cond_init(&cond_, NULL);
    }
    virtual ~Cond() {
        pthread_cond_destroy(&cond_);
    }
    int Wait() {
        return pthread_cond_wait(&cond_, &(lock_.mtx_));
    }
    int Signal() {
        return pthread_cond_signal(&cond_);
    }
    int Broadcast() {
        return pthread_cond_broadcast(&cond_);
    }

private:
    pthread_cond_t cond_;
    Mutex& lock_;
};
}

#endif //#ifndef _KV_DB_UTILS_H_
