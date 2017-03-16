//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include "BlockDevice.h"
#include "KernelDevice.h"

namespace kvdb {
BlockDevice* BlockDevice::CreateDevice() {
    return new KernelDevice();
}
}//namespace kvdb
