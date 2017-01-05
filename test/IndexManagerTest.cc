#include <string>
#include <iostream>

#include "../Kvdb_Impl.h"
#include "gtest/gtest.h"

using namespace std;
using namespace kvdb;

#define FILENAME  "/dev/loop2"

class IndexManagerTest : public ::testing::Test{

};


TEST_F(IndexManagerTest, InitIndexForCreateDB)
{
	IndexManager* idxMgr_;
	SuperBlockManager* sbMgr_;
	BlockDevice* bdev_;
	SegmentManager* segMgr_;

	bdev_ = BlockDevice::CreateDevice();
    sbMgr_ = new SuperBlockManager(bdev_);
    segMgr_ = new SegmentManager(bdev_, sbMgr_);
    idxMgr_ = new IndexManager(bdev_, sbMgr_, segMgr_);
	
	 uint64_t offset=0;
	 uint32_t numObjects=10;
	 EXPECT_TRUE(idxMgr_->InitIndexForCreateDB(offset,numObjects));
	 
}
TEST_F(IndexManagerTest, LoadIndexFromDevice)
{
	IndexManager* idxMgr_;
	SuperBlockManager* sbMgr_;
	BlockDevice* bdev_;
	SegmentManager* segMgr_;

	bdev_ = BlockDevice::CreateDevice();
    sbMgr_ = new SuperBlockManager(bdev_);
    segMgr_ = new SegmentManager(bdev_, sbMgr_);
    idxMgr_ = new IndexManager(bdev_, sbMgr_, segMgr_);
	
	 uint64_t offset=0;
	 uint32_t ht_size=10;
	 EXPECT_TRUE(idxMgr_->LoadIndexFromDevice(offset,ht_size));
	 
}
TEST_F(IndexManagerTest, WriteIndexToDevice)
{
	IndexManager* idxMgr_;
	SuperBlockManager* sbMgr_;
	BlockDevice* bdev_;
	SegmentManager* segMgr_;

	bdev_ = BlockDevice::CreateDevice();
    sbMgr_ = new SuperBlockManager(bdev_);
    segMgr_ = new SegmentManager(bdev_, sbMgr_);
    idxMgr_ = new IndexManager(bdev_, sbMgr_, segMgr_);
	

	 EXPECT_TRUE(idxMgr_->WriteIndexToDevice());
	 
}

TEST_F(IndexManagerTest, UpdateIndex)
{
	IndexManager* idxMgr_;
	SuperBlockManager* sbMgr_;
	BlockDevice* bdev_;
	SegmentManager* segMgr_;

	bdev_ = BlockDevice::CreateDevice();
    sbMgr_ = new SuperBlockManager(bdev_);
    segMgr_ = new SegmentManager(bdev_, sbMgr_);
    idxMgr_ = new IndexManager(bdev_, sbMgr_, segMgr_);
	
	 KVSlice* kvslice;
	 
	 EXPECT_TRUE(idxMgr_->UpdateIndex(kvslice));
	 
}


TEST_F(IndexManagerTest, RemoveEntry)
{
	IndexManager* idxMgr_;
	SuperBlockManager* sbMgr_;
	BlockDevice* bdev_;
	SegmentManager* segMgr_;

	bdev_ = BlockDevice::CreateDevice();
    sbMgr_ = new SuperBlockManager(bdev_);
    segMgr_ = new SegmentManager(bdev_, sbMgr_);
    idxMgr_ = new IndexManager(bdev_, sbMgr_, segMgr_);
	
	 HashEntry entry;
	 
	 idxMgr_->RemoveEntry(entry);//TODO Expection
	
	 
}
TEST_F(IndexManagerTest, GetHashEntry)
{
	IndexManager* idxMgr_;
	SuperBlockManager* sbMgr_;
	BlockDevice* bdev_;
	SegmentManager* segMgr_;

	bdev_ = BlockDevice::CreateDevice();
    sbMgr_ = new SuperBlockManager(bdev_);
    segMgr_ = new SegmentManager(bdev_, sbMgr_);
    idxMgr_ = new IndexManager(bdev_, sbMgr_, segMgr_);
	
	 KVSlice* kvslice;
	 
	 EXPECT_TRUE(idxMgr_->GetHashEntry(kvslice));
	 
}

TEST_F(IndexManagerTest, ComputeIndexSizeOnDevice)
{
	
	 uint32_t ht_size=10;
	 EXPECT_EQ(10,IndexManager::ComputeIndexSizeOnDevice(ht_size));
	 
}






int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

