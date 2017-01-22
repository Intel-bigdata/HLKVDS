#include <string>
#include <iostream>

#include "../src/Kvdb_Impl.h"
#include "../src/Options.h"
#include "gtest/gtest.h"

using namespace std;
using namespace kvdb;

#define FILENAME  "/dev/loop2"

class IndexManagerTest : public ::testing::Test{

};

IndexManager* idxMgr_;
SuperBlockManager* sbMgr_;
BlockDevice* bdev_;
SegmentManager* segMgr_;

Options opts;

void init()
{
	bdev_ = BlockDevice::CreateDevice();
    sbMgr_ = new SuperBlockManager(bdev_,opts);
    segMgr_ = new SegmentManager(bdev_, sbMgr_,opts);
    idxMgr_ = new IndexManager(bdev_, sbMgr_, segMgr_,opts);
}



TEST_F(IndexManagerTest, InitIndexForCreateDB)
{
	init();
	 uint64_t offset=0;
	 uint32_t numObjects=10;
	 EXPECT_TRUE(idxMgr_->InitIndexForCreateDB(offset,numObjects));
	 
}
TEST_F(IndexManagerTest, LoadIndexFromDevice)
{
	init();
	 uint64_t offset=0;
	 uint32_t ht_size=10;
	 EXPECT_TRUE(idxMgr_->LoadIndexFromDevice(offset,ht_size));
	 
}
TEST_F(IndexManagerTest, WriteIndexToDevice)
{
	init();
	EXPECT_TRUE(idxMgr_->WriteIndexToDevice());
	 
}

TEST_F(IndexManagerTest, UpdateIndex)
{
	
	init();
	 KVSlice* kvslice;
	 
	 EXPECT_TRUE(idxMgr_->UpdateIndex(kvslice));
	 
}


TEST_F(IndexManagerTest, RemoveEntry)
{
	init();
	 HashEntry entry;
	 
	 idxMgr_->RemoveEntry(entry);//TODO Expection
	
	 
}
TEST_F(IndexManagerTest, GetHashEntry)
{
	init();
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

