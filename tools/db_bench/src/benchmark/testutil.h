//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef DB_BENCH_TESTUTIL_H
#define DB_BENCH_TESTUTIL_H
#include "env.h"
#include "slice.h"
#include "random.h"

namespace general_db_bench {
namespace test {

// Store in *dst a random string of length "len" and return a Slice that
// references the generated data.
extern Slice RandomString(Random* rnd, int len, std::string* dst);

// Return a random key with the specified length that may contain interesting
// characters (e.g. \x00, \xff, etc.).
extern std::string RandomKey(Random* rnd, int len);

// Store in *dst a string of length "len" that will compress to
// "N*compressed_fraction" bytes and return a Slice that references
// the generated data.
extern Slice CompressibleString(Random* rnd, double compressed_fraction,
                                int len, std::string* dst);

// A wrapper that allows injection of errors.
class ErrorEnv : public EnvWrapper {
public:
    bool writable_file_error_;
    int num_writable_file_errors_;

    ErrorEnv() :
        EnvWrapper(Env::Default()), writable_file_error_(false),
                num_writable_file_errors_(0) {
    }

    virtual Status NewWritableFile(const std::string& fname,
                                   WritableFile** result) {
        if (writable_file_error_) {
            ++num_writable_file_errors_;
            *result = NULL;
            return Status::IOError(fname, "fake error");
        }
        return target()->NewWritableFile(fname, result);
    }
};

} // namespace test
} // namespace leveldb
#endif
