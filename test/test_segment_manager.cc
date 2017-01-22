#include <string>
#include <iostream>

#include "../src/Kvdb_Impl.h"
#include "gtest/gtest.h"

using namespace std;
using namespace kvdb;

#define FILENAME  "/dev/loop2"

class SegmentTest : public ::testing::Test{

};


SuperBlockManager* sbMgr_;
BlockDevice* bdev_;
SegmentManager* segMgr_;

Options opts;


void init()
{
	 bdev_ = BlockDevice::CreateDevice();
	 sbMgr_ = new SuperBlockManager(bdev_,opts);
	 segMgr_ = new SegmentManager(bdev_, sbMgr_,opts);
}

TEST_F(SegmentTest,ComputeSegTableSizeOnDisk)
{
	uint32_t seg_num=10;
	EXPECT_EQ(4096,SegmentManager::ComputeSegTableSizeOnDisk(seg_num));
	
}


TEST_F(SegmentTest, ComputeSegNum)
{
	 uint64_t total_size=40960;
	 uint64_t seg_size=4096;
	 uint32_t seg_num=SegmentManager::ComputeSegNum(total_size,seg_size);
	 std::cout<<"segment number:"<<seg_num<<std::endl;
	 EXPECT_EQ(9,seg_num);
	 
}

/*TEST_F(SegmentTest, ZeroSegSize)
{
	 uint64_t total_size=40960;
	 uint64_t seg_size=0;
	 uint32_t seg_num=SegmentManager::ComputeSegNum(total_size,seg_size);
	 std::cout<<"segment number:"<<seg_num<<std::endl;
}*/

TEST_F(SegmentTest, LargerSegSize)
{
	 uint64_t total_size=40960;
	 uint64_t seg_size=50000;
	 uint32_t seg_num=SegmentManager::ComputeSegNum(total_size,seg_size);
	 std::cout<<"segment number:"<<seg_num<<std::endl;
}



TEST_F(SegmentTest, InitSegment)
{
	init();	 
	 uint64_t total_size=40960;
	 uint64_t seg_size=4096;
	 uint32_t seg_num=SegmentManager::ComputeSegNum(total_size,seg_size);
	 std::cout<<"segment number:"<<seg_num<<std::endl;

	 uint64_t segtable_offset=0;
	
	 if(segMgr_->InitSegmentForCreateDB(segtable_offset,seg_size,seg_num)){
		 std::cout<<"true"<<std::endl;


		EXPECT_EQ(9,segMgr_->GetTotalFreeSegs());
		EXPECT_EQ(0,segMgr_->GetTotalUsedSegs());
		//EXPECT_EQ(0,segMgr_->LoadSegmentTableFromDevice(start_offset,uint32_t segment_size,uint32_t num_seg,uint32_t current_seg)):
	 }


}

TEST_F(SegmentTest, LoadSegmentTableFromDevice)
{
	init();
	uint64_t start_offset=0;
	uint32_t segment_size=4096;
	uint32_t num_seg=9;
	uint32_t current_seg=1;
	
	 EXPECT_TRUE(segMgr_->LoadSegmentTableFromDevice(start_offset,segment_size,num_seg,current_seg));
	 
}


TEST_F(SegmentTest, WriteSegmentTableToDevice)
{
	init();
	 EXPECT_TRUE(segMgr_->WriteSegmentTableToDevice());
	 
}

TEST_F(SegmentTest, ComputeSegOffsetFromOffset)
{
	init();
	uint64_t offset=1;
	uint64_t seg_offset=2;
	 EXPECT_TRUE(segMgr_->ComputeSegOffsetFromOffset(offset,seg_offset));
	 
}

TEST_F(SegmentTest, ComputeDataOffsetPhyFromEntry)
{
	init();

	HashEntry *entry;
	entry=new HashEntry();
	uint64_t data_offset=1;
	EXPECT_TRUE(segMgr_->ComputeDataOffsetPhyFromEntry(entry,data_offset));
	 
}



int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


