#ifndef _HLKVDS_STATUS_H_
#define _HLKVDS_STATUS_H_

#include <string>
namespace hlkvds {

class Status {
public:
    Status() :
        code_(kOk), state_(nullptr) {
    }
    ~Status() {
	if(!state_){
            delete[] state_;
	}
    }

    enum Code {
        kOk = 0,
        kNotFound = 1,
        kCorruption = 2,
        kNotSupported = 3,
        kInvalidArgument = 4,
        kIOError = 5,
        kTimedOut = 6,
        kAborted = 7,
        kBusy = 8,
        kTryAgain = 9
    };
    Code code() const {
        return code_;
    }

    static Status OK() {
        return Status();
    }
    static Status NotFound(const char* msg) {
        return Status(kNotFound, msg);
    }
    static Status Corruption(const char* msg) {
        return Status(kCorruption, msg);
    }
    static Status NotSupported(const char* msg) {
        return Status(kNotSupported, msg);
    }
    static Status InvalidArgument(const char* msg) {
        return Status(kInvalidArgument, msg);
    }
    static Status IOError(const char* msg) {
        return Status(kIOError, msg);
    }
    static Status TimedOut(const char* msg) {
        return Status(kTimedOut, msg);
    }
    static Status Aborted(const char* msg) {
        return Status(kAborted, msg);
    }
    static Status Busy(const char* msg) {
        return Status(kBusy, msg);
    }
    static Status TryAgain(const char* msg) {
        return Status(kTryAgain, msg);
    }

    bool ok() const {
        return code() == kOk;
    }
    bool notfound() const {
        return code() == kNotFound;
    }
    std::string ToString() const;
private:
    Code code_;
    const char* state_;
    Status(Code _code, const char* msg);

};
}
#endif //#define _HLKVDS_STATUS_H_
