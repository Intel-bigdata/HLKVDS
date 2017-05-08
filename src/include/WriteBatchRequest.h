/*
 * WriteBatchRequest.h
 *
 *  Created on: Mar 30, 2017
 *      Author: yongfu
 */

#ifndef WRITEBATCHREQUEST_H_
#define WRITEBATCHREQUEST_H_

#include "DataHandle.h"

namespace kvdb {
class WriteBatchRequest : public IRequest {
public:
    WriteBatchRequest();
    WriteBatchRequest(const WriteBatchRequest& toBeCopied);
    WriteBatchRequest& operator=(const WriteBatchRequest& toBeCopied);
    ~WriteBatchRequest();

    virtual KVSlice& GetSlice(){
    }
    virtual void addSlice(KVSlice& slice);
    virtual std::list<KVSlice *>& getSlices(){
        return *slices_;
    }


private:
    //uint32_t total;
    //std::list<KVSlice *> *slices_;
};
}

#endif /* WRITEBATCHREQUEST_H_ */
