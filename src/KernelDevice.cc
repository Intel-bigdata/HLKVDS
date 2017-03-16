//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <string.h>
#include <stdlib.h>

#include <vector>
#include "KernelDevice.h"

namespace kvdb {
KernelDevice::KernelDevice() :
    directFd_(-1), bufFd_(-1), capacity_(0), blockSize_(0), path_("") {
}

KernelDevice::~KernelDevice() {
    if (directFd_ != -1 || bufFd_ != -1) {
        Close();
    }
}

int KernelDevice::SetNewDBZero(off_t meta_size, bool clear_data_region) {
    int r = set_metazone_zero(meta_size);
    if (r < 0) {
        __ERROR("couldn't set metazone zero");
        return ERR;
    }

    if (clear_data_region) {
        r = set_device_zero();
        if (r < 0) {
            __ERROR("couldn't set device zero");
            return ERR;
        }
    }

    return FOK;
}

int KernelDevice::set_device_zero() {
    int r = 0;

    lseek(bufFd_, 0, SEEK_SET);
    r = fill_file_with_zeros();
    if (r < 0) {
        __ERROR("Could not zero out DB file.\n");
        return ERR;
    }

    return FOK;
}

int KernelDevice::set_metazone_zero(uint64_t meta_size) {
    size_t nbytes = meta_size;

    static const size_t BLOCKSIZE = 8192;
    char zeros[BLOCKSIZE];
    memset(zeros, 0, BLOCKSIZE);
    while (nbytes > 0) {
        size_t bytes_to_write = min(nbytes, BLOCKSIZE);
        ssize_t ret = write(bufFd_, zeros, bytes_to_write);
        if (ret < 0) {
            __ERROR("error in fill_file_with_zeros write: %s\n", strerror(errno));
            return ERR;
        }
        nbytes -= bytes_to_write;
    }
    return FOK;
}

int KernelDevice::fill_file_with_zeros() {
    size_t nbytes = capacity_;

    static const size_t BLOCKSIZE = 8192;
    char zeros[BLOCKSIZE];
    memset(zeros, 0, BLOCKSIZE);
    while (nbytes > 0) {
        size_t bytes_to_write = min(nbytes, BLOCKSIZE);
        ssize_t ret = write(bufFd_, zeros, bytes_to_write);
        if (ret < 0) {
            __ERROR("error in fill_file_with_zeros write: %s\n", strerror(errno));
            return ERR;
        }
        nbytes -= bytes_to_write;
    }
    return FOK;
}

uint64_t KernelDevice::get_block_device_capacity() {
    uint64_t blocksize;
    if (ioctl(bufFd_, BLKGETSIZE64, &blocksize) < 0) {
        __ERROR("Could not get block size: %s\n", strerror(errno));
        return ERR;
    }

    return blocksize;
}

int KernelDevice::Open(string path, bool dsync) {
    path_ = path;

    int r = 0;

    directFd_ = open(path_.c_str(), O_RDWR | O_DIRECT, 0666);
    if (directFd_ < 0) {
        __ERROR("Could not open file: %s\n", strerror(errno));
        goto open_fail;
    }

    if (dsync) {
        //bufFd_ = open(path_.c_str(), O_RDWR | O_SYNC, 0666);
        bufFd_ = open(path_.c_str(), O_RDWR | O_DSYNC, 0666);
    } else {
        bufFd_ = open(path_.c_str(), O_RDWR, 0666);
    }

    if (bufFd_ < 0) {
        __ERROR("Could not open file: %s\n", strerror(errno));
        goto open_fail;
    }

    r = posix_fadvise(bufFd_, 0, 0, POSIX_FADV_RANDOM);
    if (r < 0) {
        __ERROR("Couldn't posix_fadvise random: %s\n", strerror(errno));
        goto open_fail;
    }

    struct stat statbuf;
    r = fstat(directFd_, &statbuf);
    if (r < 0) {
        __ERROR("Couldn't read fstat: %s\n", strerror(errno));
        goto open_fail;
    }

    if (S_ISBLK(statbuf.st_mode)) {
        capacity_ = get_block_device_capacity();
        blockSize_ = statbuf.st_blksize;
    } else {
        __ERROR("The path is not a Block device.");
        goto open_fail;
    }
    return FOK;
    open_fail: close(bufFd_);
    close(directFd_);
    directFd_ = -1;
    bufFd_ = -1;
    return ERR;

}

void KernelDevice::Close() {
    if (directFd_ != -1) {
        close(directFd_);
    }
    if (bufFd_ != -1) {
        close(bufFd_);
    }
}

ssize_t KernelDevice::pWrite(const void* buf, size_t count, off_t offset) {
    return pwrite(bufFd_, buf, count, offset);
}

ssize_t KernelDevice::pRead(void* buf, size_t count, off_t offset) {
    return pread(bufFd_, buf, count, offset);
}

ssize_t KernelDevice::pWritev(const struct iovec *iov, int iovcnt, off_t offset) {
    return pwritev(bufFd_, iov, iovcnt, offset);
}

ssize_t KernelDevice::pReadv(const struct iovec *iov, int iovcnt, off_t offset) {
    return preadv(bufFd_, iov, iovcnt, offset);
}

void KernelDevice::ClearReadCache() {
    posix_fadvise(bufFd_, 0, capacity_, POSIX_FADV_DONTNEED);
}
}
