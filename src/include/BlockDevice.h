//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef KV_DB_BLOCKDEVICE_H_
#define KV_DB_BLOCKDEVICE_H_

#include <sys/types.h>
#include <stdint.h>
#include <sys/uio.h>
#include <string>
#include "Db_Structure.h"

using namespace std;

namespace kvdb {
class BlockDevice {
public:
    static BlockDevice* CreateDevice();

    BlockDevice() {
    }
    virtual ~BlockDevice() {
    }

    virtual int
            SetNewDBZero(off_t meta_size, bool clear_data_region = false) = 0;
    virtual int Open(string path, bool dsync = true) = 0;
    virtual void Close() = 0;

    virtual uint64_t GetDeviceCapacity() = 0;

    virtual ssize_t pWrite(const void* buf, size_t count, off_t offset) = 0;
    virtual ssize_t pRead(void* buf, size_t count, off_t offset) = 0;
    virtual ssize_t
            pWritev(const struct iovec *iov, int iovcnt, off_t offset) = 0;
    virtual ssize_t
            pReadv(const struct iovec *iov, int iovcnt, off_t offset) = 0;

    virtual void ClearReadCache() = 0;
};
}//namespace kvdb

#endif // #ifndef _KV_DB_BLOCKDEVICE_H_
