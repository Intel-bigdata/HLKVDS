#include <string>
#include <iostream>
#include "test_base.h"

class test_cache : public TestBase {
public:
    dslab::ReadCache* SLRUCache_;
    dslab::ReadCache* LRUCache_;

    virtual void SetUp() {
        SLRUCache_ = new dslab::ReadCache(dslab::SLRU, 4, 50);
        LRUCache_ = new dslab::ReadCache(dslab::LRU, 2,0);
    }
    ~test_cache(){
	delete SLRUCache_;
	delete LRUCache_;
    }
};

TEST_F(test_cache, SLRUOperations){
    string data = "";
    SLRUCache_->Put("1",">");
    SLRUCache_->Put("2","`");
    EXPECT_TRUE(SLRUCache_->Get("1",data));
    EXPECT_EQ(">",data);

    SLRUCache_->Put("3","fdsj");
    SLRUCache_->Put("4","123");
    EXPECT_FALSE(SLRUCache_->Get("2",data));
    
    EXPECT_TRUE(SLRUCache_->Get("3",data));
    SLRUCache_->Delete("3");
    EXPECT_FALSE(SLRUCache_->Get("3",data));
}

TEST_F(test_cache, LRUOperations){
    string data = "";
    LRUCache_->Put("1",">");
    LRUCache_->Put("2","`");
    EXPECT_TRUE(LRUCache_->Get("1",data));
    EXPECT_EQ(">",data);

    LRUCache_->Put("3","fdsj");
    EXPECT_FALSE(LRUCache_->Get("2",data));
    
    EXPECT_TRUE(LRUCache_->Get("3",data));
    LRUCache_->Delete("3");
    EXPECT_FALSE(LRUCache_->Get("3",data));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
