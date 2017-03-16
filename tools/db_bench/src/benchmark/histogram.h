//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef DB_BENCH_HISTOGRAM_H
#define DB_BENCH_HISTOGRAM_H
#include <string>

namespace general_db_bench {

class Histogram {
public:
    Histogram() {
    }
    ~Histogram() {
    }

    void Clear();
    void Add(double value);
    void Merge(const Histogram& other);

    std::string ToString() const;

private:
    double min_;
    double max_;
    double num_;
    double sum_;
    double sum_squares_;

    enum {
        kNumBuckets = 154
    };
    static const double kBucketLimit[kNumBuckets];
    double buckets_[kNumBuckets];

    double Median() const;
    double Percentile(double p) const;
    double Average() const;
    double StandardDeviation() const;
};

} // namespace leveldb

#endif
