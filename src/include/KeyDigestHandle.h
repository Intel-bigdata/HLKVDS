//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef _KV_DB_KEYDIGESTHANDLE_H_
#define _KV_DB_KEYDIGESTHANDLE_H_

#include <string.h>
#include <stdint.h>
#include <string>
#include "rmd160.h"

#include "Db_Structure.h"

//#define RMDsize 160
//#define DIGEST_LEN RMDsize/8

using namespace std;

namespace kvdb {
class Kvdb_Key {
private:
    const char* value;
    uint32_t len;

public:
    uint32_t GetLen() const {
        return len;
    }
    const char* GetValue() const {
        return value;
    }
    Kvdb_Key() :
        value(NULL), len(0) {
    }
    Kvdb_Key(const char* key, uint32_t key_len);
    Kvdb_Key(const Kvdb_Key& toBeCopied);
    ~Kvdb_Key();
}__attribute__((__packed__));

class Kvdb_Digest {
private:
    friend class KeyDigestHandle;
    uint32_t value[KEYDIGEST_INT_NUM];

public:
    Kvdb_Digest();
    ~Kvdb_Digest();
    Kvdb_Digest(const Kvdb_Digest &toBeCopied);
    Kvdb_Digest& operator=(const Kvdb_Digest& toBeCopied);
    bool operator==(const Kvdb_Digest& toBeCompare) const;
    void SetDigest(unsigned char* data, int len);
    unsigned char* GetDigest() const {
        return (unsigned char*) value;
    }
}__attribute__((__packed__));

class KeyDigestHandle {
public:
    static inline size_t SizeOfDigest() {
        return sizeof(Kvdb_Digest);
    }

    static uint32_t Hash(const Kvdb_Key *key);
    static uint32_t Hash(const Kvdb_Digest *digest);
    static void ComputeDigest(const Kvdb_Key *key, Kvdb_Digest &digest);
    static string Tostring(Kvdb_Digest *digest);

private:
    KeyDigestHandle();

};
}// namespace kvdb

#endif //#ifndef _KV_DB_KEYDIGESTHANDLE_H_
