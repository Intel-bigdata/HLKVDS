//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include "dbgtracer.h"

Tracer* Tracer::getInstance() {
    static Tracer* inst = NULL;
    if (inst == NULL) {
        mu_.Lock();
        if (inst == NULL) {
            inst = new Tracer();
        }
        mu_.Unlock();
    }
    return inst;
}
