//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once
#ifndef TEST_BASE_H_
#define TEST_BASE_H_

#include "Kvdb_Impl.h"
#include "hyperds/Options.h"
#include "gtest/gtest.h"
#include <string>

#define FILENAME  "/dev/loop2"
using namespace kvdb;

class TestBase : public ::testing::Test {
public:
    KvdbDS* Create_DB(int db_size);
protected:
    Options opts;

};

#endif  //TEST_BASE_H_
