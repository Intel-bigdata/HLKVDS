//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <string>
#include <iostream>
#include "test_base.h"

class test_block_manager : public TestBase {
public:
    SuperBlockManager* sbMgr_;
    BlockDevice* bdev_;
    Options opts;

    virtual void SetUp() {
        bdev_ = BlockDevice::CreateDevice();
        sbMgr_ = new SuperBlockManager(bdev_, opts);
        bdev_->Open(FILENAME);
    }
};

TEST_F(test_block_manager, InitSuperBlockForCreateDB) {
    uint64_t offset = 0;
    EXPECT_TRUE(sbMgr_->InitSuperBlockForCreateDB(offset));
    EXPECT_TRUE(sbMgr_->WriteSuperBlockToDevice());
    EXPECT_TRUE(sbMgr_->LoadSuperBlockFromDevice(offset));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

