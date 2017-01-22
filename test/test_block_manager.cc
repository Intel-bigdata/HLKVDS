#include <string>
#include <iostream>

#include "../src/Kvdb_Impl.h"
#include "gtest/gtest.h"

using namespace std;
using namespace kvdb;

#define FILENAME  "/dev/loop2"

class SuperBlockManagerTest: public ::testing::Test{

};

SuperBlockManager* sbMgr_;
BlockDevice* bdev_;
Options opts;

void init()
{
	bdev_ = BlockDevice::CreateDevice();
    sbMgr_ = new SuperBlockManager(bdev_,opts);
}


TEST_F(SuperBlockManagerTest, InitSuperBlockForCreateDB)
{
	init();
	 uint64_t offset=0;
	 EXPECT_TRUE(sbMgr_->InitSuperBlockForCreateDB(offset));
	 
}
TEST_F(SuperBlockManagerTest, LoadSuperBlockFromDevice)
{

init();
	 uint64_t offset=0;
	 EXPECT_TRUE(sbMgr_->LoadSuperBlockFromDevice(offset));
	 
}

TEST_F(SuperBlockManagerTest, WriteSuperBlockToDevice)
{

	init();
	 EXPECT_TRUE(sbMgr_->WriteSuperBlockToDevice());
	 
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

