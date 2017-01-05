#include <string>
#include <iostream>

#include "../Kvdb_Impl.h"
#include "gtest/gtest.h"

using namespace std;
using namespace kvdb;

#define FILENAME  "/dev/loop2"

class SuperBlockManagerTest: public ::testing::Test{

};


TEST_F(SuperBlockManagerTest, InitSuperBlockForCreateDB)
{
	SuperBlockManager* sbMgr_;
	BlockDevice* bdev_;


	bdev_ = BlockDevice::CreateDevice();
    sbMgr_ = new SuperBlockManager(bdev_);
  
	
	 uint64_t offset=0;
	 EXPECT_TRUE(sbMgr_->InitSuperBlockForCreateDB(offset));
	 
}
TEST_F(SuperBlockManagerTest, LoadSuperBlockFromDevice)
{

	SuperBlockManager* sbMgr_;
	BlockDevice* bdev_;


	bdev_ = BlockDevice::CreateDevice();
    sbMgr_ = new SuperBlockManager(bdev_);
  
	
	 uint64_t offset=0;
	 EXPECT_TRUE(sbMgr_->LoadSuperBlockFromDevice(offset));
	 
}

TEST_F(SuperBlockManagerTest, WriteSuperBlockToDevice)
{

	SuperBlockManager* sbMgr_;
	BlockDevice* bdev_;
	

	bdev_ = BlockDevice::CreateDevice();
    sbMgr_ = new SuperBlockManager(bdev_);
  
	 EXPECT_TRUE(sbMgr_->WriteSuperBlockToDevice());
	 
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

