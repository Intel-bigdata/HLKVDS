#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <cstring>
#include "hlkvds/Status.h"
#include "Db_Structure.h"

namespace hlkvds {

Status::Status(Code _code, const char* msg) :
    code_(_code) {
    uint32_t size = strlen(msg);
    char* const result = new char[size + 1];
    memcpy(result, msg, size);
    result[size] = '\0';
    state_ = result;
}

std::string Status::ToString() const {
    char tmp[30];
    const char* type;
    switch (code_) {
        case kOk:
            return "OK";
        case kNotFound:
            type = "NotFound: ";
            break;
        case kCorruption:
            type = "Corruption: ";
            break;
        case kNotSupported:
            type = "Not implemented: ";
            break;
        case kInvalidArgument:
            type = "Invalid argument: ";
            break;
        case kIOError:
            type = "IO error: ";
            break;
        case kTimedOut:
            type = "Operation timed out: ";
            break;
        case kAborted:
            type = "Operation aborted: ";
            break;
        case kBusy:
            type = "Resource busy: ";
            break;
        case kTryAgain:
            type = "Operation failed. Try again.: ";
            break;
        default:
            snprintf(tmp, sizeof(tmp), "Unknown code(%d): ",
                     static_cast<int> (code()));
            type = tmp;
            break;
    }
    std::string result(type);
    if (state_ != nullptr) {
        result.append(state_);
    }
    return result;
}
}
