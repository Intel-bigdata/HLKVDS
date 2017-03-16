//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <set>
#include <iomanip>
#include "test_base.h"
#include "KeyDigestHandle.h"

class test_rmd : public TestBase {

};

TEST_F(test_rmd, IntKeyTest)
{
    uint32_t conflict = 0;
    set<string> result_set;

    for(uint32_t i=0; i< 4294967295; i++)
    {
        char *pi = (char *)&i;

        char *key_char = new char[4];
        key_char[0] = *((char *)pi+0);
        key_char[1] = *((char *)pi+1);
        key_char[2] = *((char *)pi+2);
        key_char[3] = *((char *)pi+3);

        kvdb::Kvdb_Key key(key_char, 4);
        kvdb::Kvdb_Digest result;
        KeyDigestHandle::ComputeDigest(&key, result);

        string final_res = KeyDigestHandle::Tostring(&result);

        if(result_set.count(final_res))
        {
            conflict ++;
            cout<< i << key_char[0] << key_char[1] << key_char[2] << key_char[3] << "result: " << final_res <<" is exist" <<endl;
        }
        else
        {
            result_set.insert(final_res);
        }

        delete key_char;
        key_char = NULL;

        if(i %1000000 == 0)
        cout << "now compute i is : " << i << " , conflict is : " << conflict <<endl;
    }

    cout << "conflict is :" << conflict << endl;
    return;
}

TEST_F(test_rmd, RegularTest)
{
    //char* key_raw = "";
    //char* key_raw = "a";
    //char* key_raw = "abc";
    char* key_raw = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::cout << "The \"" << key_raw << "\" hashcode:\t";

    int key_len = strlen(key_raw);

    kvdb::Kvdb_Key key(key_raw, key_len);
    kvdb::Kvdb_Digest result;
    KeyDigestHandle::ComputeDigest(&key, result);

    string final_res = KeyDigestHandle::Tostring(&result);
    std::cout << final_res << std::endl;

    printf("hash index from key is:\t");
    printf("%u", KeyDigestHandle::Hash(&key));
    printf("\n");

    printf("hash index from digest is:\t");
    printf("%u", KeyDigestHandle::Hash(&result));
    printf("\n");

}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
