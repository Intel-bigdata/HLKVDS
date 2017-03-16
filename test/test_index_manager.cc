//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <string>
#include <iostream>
#include "test_base.h"

class IndexManagerTest : public TestBase {
public:
    IndexManager* idxMgr_;
    SuperBlockManager* sbMgr_;
    BlockDevice* bdev_;
    SegmentManager* segMgr_;

    Options opts;

    virtual void SetUp() {
        bdev_ = BlockDevice::CreateDevice();
        sbMgr_ = new SuperBlockManager(bdev_, opts);
        segMgr_ = new SegmentManager(bdev_, sbMgr_, opts);
        idxMgr_ = new IndexManager(bdev_, sbMgr_, segMgr_, opts);
        bdev_->Open(FILENAME);
    }
};

TEST_F(IndexManagerTest, ComputeIndexSizeOnDevice)
{
    uint32_t ht_size=10;
    EXPECT_EQ(4096,IndexManager::ComputeIndexSizeOnDevice(ht_size));

}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

