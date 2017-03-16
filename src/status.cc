//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <cstring>
#include "hyperds/status.h"

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
