/*
 * WriteBatchRequest.cc
 *
 *  Created on: Mar 30, 2017
 *      Author: yongfu
 */

#include "WriteBatchRequest.h"

namespace kvdb {

WriteBatchRequest::WriteBatchRequest() {
    slices_ = new std::list<KVSlice *>;
}

void WriteBatchRequest::addSlice(KVSlice& slice) {
    //total += slice->GetDataLen() + IndexManager::SizeOfDataHeader();

    slices_->push_back(&slice);

}

}
