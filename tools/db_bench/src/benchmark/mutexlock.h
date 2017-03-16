//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef DB_BENCH_MUTEXLOCK_H
#define DB_BENCH_MUTEXLOCK_H
#include "port_posix.h"

namespace general_db_bench {

// Helper class that locks a mutex on construction and unlocks the mutex when
// the destructor of the MutexLock object is invoked.
//
// Typical usage:
//
//   void MyClass::MyMethod() {
//     MutexLock l(&mu_);       // mu_ is an instance variable
//     ... some complex code, possibly with multiple return paths ...
//   }

class MutexLock {
public:
    explicit MutexLock(port::Mutex *mu) :
        mu_(mu) {
        this->mu_->Lock();
    }
    ~MutexLock() {
        this->mu_->Unlock();
    }

private:
    port::Mutex * const mu_;
    // No copying allowed
    MutexLock(const MutexLock&);
    void operator=(const MutexLock&);
};

} // namespace general_db_bench


#endif  // 
