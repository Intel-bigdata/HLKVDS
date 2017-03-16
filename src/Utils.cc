//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <stdio.h>
#include "Utils.h"

namespace kvdb {

const char* KVTime::ToChar(KVTime& _time) {
    return asctime(localtime(&_time.tm_.tv_sec));
}

time_t KVTime::GetNow() {
    timeval now;
    gettimeofday(&now, NULL);
    return (time_t) now.tv_sec;
}

const char* KVTime::GetNowChar() {
    timeval now;
    gettimeofday(&now, NULL);
    return asctime(localtime(&now.tv_sec));
}

void KVTime::SetTime(time_t _time) {
    tm_.tv_sec = _time;
    tm_.tv_usec = 0;
}

time_t KVTime::GetTime() {
    return (time_t) tm_.tv_sec;
}

timeval KVTime::GetTimeval() {
    return tm_;
}

void KVTime::Update() {
    gettimeofday(&tm_, NULL);
}

KVTime::KVTime() {
    gettimeofday(&tm_, NULL);
}

KVTime::KVTime(const KVTime& toBeCopied) {
    tm_ = toBeCopied.tm_;
}

KVTime& KVTime::operator=(const KVTime& toBeCopied) {
    tm_ = toBeCopied.tm_;
    return *this;
}

bool KVTime::operator>(const KVTime& toBeCopied) {
    if (tm_.tv_sec > toBeCopied.tm_.tv_sec) {
        return true;
    } else if (tm_.tv_sec < toBeCopied.tm_.tv_sec) {
        return false;
    } else {
        return tm_.tv_usec > toBeCopied.tm_.tv_usec;
    }
}

bool KVTime::operator<(const KVTime& toBeCopied) {
    if (tm_.tv_sec < toBeCopied.tm_.tv_sec) {
        return true;
    } else if (tm_.tv_sec > toBeCopied.tm_.tv_sec) {
        return false;
    } else {
        return tm_.tv_usec < toBeCopied.tm_.tv_usec;
    }
}

bool KVTime::operator==(const KVTime& toBeCopied) {
    return (tm_.tv_sec == toBeCopied.tm_.tv_sec && tm_.tv_usec
            == toBeCopied.tm_.tv_usec);
}

int64_t KVTime::operator-(const KVTime& toBeCopied) {
    int64_t sec_diff = tm_.tv_sec - toBeCopied.tm_.tv_sec;
    int64_t usec_diff = tm_.tv_usec - toBeCopied.tm_.tv_usec;
    return sec_diff * 1000000 + usec_diff;
}

KVTime::~KVTime() {
}

void* Thread::runThread(void* arg) {
    return ((Thread*) arg)->Entry();
}

Thread::Thread() :
    tid_(0), running_(0), detached_(0) {
}

Thread::~Thread() {
}

int Thread::Start() {
    int result = pthread_create(&tid_, NULL, runThread, (void *) this);
    if (result == 0) {
        running_ = 1;
    }
    return result;
}

int Thread::Join() {
    int result = -1;
    if (running_ == 1) {
        result = pthread_join(tid_, NULL);
        if (result == 0) {
            detached_ = 0;
        }
    }
    return result;
}

int Thread::Detach() {
    int result = -1;
    if (running_ == 1 && detached_ == 0) {
        result = pthread_detach(tid_);
        if (result == 0) {
            detached_ = 1;
        }
    }
    return result;
}

bool Thread::Is_started() const {
    return tid_ != 0;
}

bool Thread::Am_self() const {
    return (pthread_self() == tid_);
}

pthread_t Thread::Self() {
    return tid_;
}
} // namespace kvdb
