//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#ifndef KV_DB_KERNELDEVICE_H_
#define KV_DB_KERNELDEVICE_H_

#include <string>
#include <unistd.h>

#include "BlockDevice.h"

namespace kvdb {

class KernelDevice : public BlockDevice {
public:
    KernelDevice();
    virtual ~KernelDevice();

    int SetNewDBZero(off_t meta_size, bool clear_data_region);
    int Open(string path, bool dsync);
    void Close();
    void ClearReadCache();

    uint64_t GetDeviceCapacity() {
        return get_capacity();
    }

    ssize_t pWrite(const void* buf, size_t count, off_t offset);
    ssize_t pRead(void* buf, size_t count, off_t offset);
    ssize_t pWritev(const struct iovec *iov, int iovcnt, off_t offset);
    ssize_t pReadv(const struct iovec *iov, int iovcnt, off_t offset);

private:
    int directFd_;
    int bufFd_;
    uint64_t capacity_;
    int blockSize_;
    std::string path_;

    int set_device_zero();
    int set_metazone_zero(uint64_t meta_size);
    int fill_file_with_zeros();
    uint64_t get_block_device_capacity();
    int disable_readahead();

    uint64_t get_capacity() {
        return capacity_;
    }
    int get_pagesize() {
        return getpagesize();
    }
    int get_blocksize() {
        return blockSize_;
    }

};

}//namespace kvdb

#endif // #ifndef KV_DB_KERNELDEVICE_H_
