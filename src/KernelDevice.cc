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
#include "Db_Structure.h"

using namespace std;

namespace hlkvds {
KernelDevice::KernelDevice() :
    directFd_(-1), bufFd_(-1), capacity_(0), blockSize_(0), path_(""), isOpen_(false) {
}

KernelDevice::~KernelDevice() {
    if (directFd_ != -1 || bufFd_ != -1) {
        Close();
    }
}

int KernelDevice::ZeroDevice() {
    return set_device_zero();
}

int KernelDevice::set_device_zero() {
    int r = 0;

    lseek(bufFd_, 0, SEEK_SET);
    r = fill_file_with_zeros();
    if (r < 0) {
        __ERROR("Could not zero out DB file.\n");
        return KD_ERR;
    }

    return KD_OK;
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
            return KD_ERR;
        }
        nbytes -= bytes_to_write;
    }
    return KD_OK;
}

uint64_t KernelDevice::get_block_device_capacity() {
    uint64_t blocksize;
    if (ioctl(bufFd_, BLKGETSIZE64, &blocksize) < 0) {
        __ERROR("Could not get block size: %s\n", strerror(errno));
        return KD_ERR;
    }

    return blocksize;
}

int KernelDevice::lock_device()
{
    struct flock l;
    memset(&l, 0, sizeof(l));
    l.l_type = F_WRLCK;
    l.l_whence = SEEK_SET;
    int r = ::fcntl(directFd_, F_SETLK, &l);
    if (r < 0) {
        return KD_ERR;
    }
    return KD_OK;
}

int KernelDevice::Open(string path, bool dsync) {
    if (isOpen_) {
        __ERROR("This Device is already Open!!!");
        return KD_ERR;
    }

    path_ = path;

    int r = 0;

    directFd_ = open(path_.c_str(), O_RDWR | O_DIRECT | O_DSYNC, 0666);
    if (directFd_ < 0) {
        __ERROR("Could not open file: %s %s\n", path_.c_str(), strerror(errno));
        goto open_fail;
    }

    if (dsync) {
        //bufFd_ = open(path_.c_str(), O_RDWR | O_SYNC, 0666);
        bufFd_ = open(path_.c_str(), O_RDWR | O_DSYNC, 0666);
    } else {
        bufFd_ = open(path_.c_str(), O_RDWR, 0666);
    }

    if (bufFd_ < 0) {
        __ERROR("Could not open file: %s %s\n", path_.c_str(), strerror(errno));
        goto open_fail;
    }

    r = posix_fadvise(bufFd_, 0, 0, POSIX_FADV_RANDOM);
    if (r < 0) {
        __ERROR("Couldn't posix_fadvise random: %s %s\n", path_.c_str(), strerror(errno));
        goto open_fail;
    }

    r = lock_device();
    if (r < 0) {
        __ERROR("failed to lock device: %s %s\n", path_.c_str(), strerror(errno));
        goto open_fail;
    }

    struct stat statbuf;
    r = fstat(directFd_, &statbuf);
    if (r < 0) {
        __ERROR("Couldn't read fstat: %s %s\n", path_.c_str(), strerror(errno));
        goto open_fail;
    }

    if (S_ISBLK(statbuf.st_mode)) {
        capacity_ = get_block_device_capacity();
        blockSize_ = statbuf.st_blksize;
    } else {
        capacity_ = statbuf.st_size;
        //TODO:() dynamic block size in file mode
        blockSize_ = getpagesize();
    }

    isOpen_ = true;

    return KD_OK;

open_fail:
    close(bufFd_);
    close(directFd_);
    directFd_ = -1;
    bufFd_ = -1;
    return KD_ERR;

}

void KernelDevice::Close() {
    isOpen_ = false;

    if (directFd_ != -1) {
        close(directFd_);
    }
    if (bufFd_ != -1) {
        close(bufFd_);
    }
}

ssize_t KernelDevice::pWrite(const void* buf, size_t count, off_t offset) {
    if (IsPageAligned(buf) &&
        IsSectorAligned(count) &&
        IsSectorAligned(offset)) {
        return DirectWriteAligned(buf, count, offset);
    }
    __INFO("Buffered FD Pwrite, write size = %ld, block_size = %d",
            count, GetBlockSize());
    //return pwrite(bufFd_, buf, count, offset);
    ssize_t ret = pwrite(bufFd_, buf, count, offset);
    fsync(bufFd_);
    return ret;
}

ssize_t KernelDevice::pRead(void* buf, size_t count, off_t offset) {
    ssize_t result = pread(bufFd_, buf, count, offset);
    posix_fadvise(bufFd_, offset, count, POSIX_FADV_DONTNEED);
    return result;
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

ssize_t KernelDevice::DirectWriteAligned(const void* buf, size_t count, off_t offset) {
    //__INFO("Direct FD Pwrite");
    return pwrite(directFd_, buf, count, offset);
}

}
