//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <string>
#include <iostream>
#include "test_base.h"

class test_segment_manager : public TestBase {
public:
    SuperBlockManager* sbMgr_;
    BlockDevice* bdev_;
    SegmentManager* segMgr_;
    Options opts;

    virtual void SetUp() {
        bdev_ = BlockDevice::CreateDevice();
        sbMgr_ = new SuperBlockManager(bdev_, opts);
        segMgr_ = new SegmentManager(bdev_, sbMgr_, opts);
        bdev_->Open(FILENAME);
    }
};

TEST_F(test_segment_manager,ComputeSegTableSizeOnDisk)
{
    uint32_t seg_num=10;
    EXPECT_EQ(4096,SegmentManager::ComputeSegTableSizeOnDisk(seg_num));

}

TEST_F(test_segment_manager, ComputeSegNum)
{
    uint64_t total_size=40960;
    uint64_t seg_size=4096;
    uint32_t seg_num=SegmentManager::ComputeSegNum(total_size,seg_size);
    EXPECT_EQ(9,seg_num);

}

TEST_F(test_segment_manager, LargerSegSize)
{
    uint64_t total_size=40960;
    uint64_t seg_size=50000;
    uint32_t seg_num=SegmentManager::ComputeSegNum(total_size,seg_size);
    EXPECT_EQ(0,seg_num);
}

TEST_F(test_segment_manager, InitSegment)
{
    uint64_t total_size=40960;
    uint64_t seg_size=4096;
    uint32_t seg_num=SegmentManager::ComputeSegNum(total_size,seg_size);

    uint64_t segtable_offset=0;

    EXPECT_TRUE(segMgr_->InitSegmentForCreateDB(segtable_offset,seg_size,seg_num));
    EXPECT_EQ(9,segMgr_->GetTotalFreeSegs());
    EXPECT_EQ(0,segMgr_->GetTotalUsedSegs());

}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

