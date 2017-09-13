#ifndef _KV_DB_KEYDIGESTHANDLE_H_
#define _KV_DB_KEYDIGESTHANDLE_H_

#include <string.h>
#include <stdint.h>
#include <string>
#include "rmd160.h"
#include "Db_Structure.h"
#define DIGEST_INT_NUM RMDsize/(sizeof(uint32_t)*8)
using namespace std;
namespace dslab{
class CryptoMgr_Input {
private:
    const char* value;
    uint32_t len;

public:
    uint32_t GetLen() const {
        return len;
    }
    const char* GetValue() const {
        return value;
    }
    CryptoMgr_Input() :
        value(NULL), len(0) {
    }
    CryptoMgr_Input(const char* input, uint32_t input_len);
    CryptoMgr_Input(const CryptoMgr_Input& toBeCopied);
    ~CryptoMgr_Input();
}__attribute__((__packed__));


class CryptoMgr_Digest {
private:
    friend class CryptoMgr;
    uint32_t value[DIGEST_INT_NUM];

public:
    CryptoMgr_Digest();
    ~CryptoMgr_Digest();
    CryptoMgr_Digest(const CryptoMgr_Digest &toBeCopied);
    CryptoMgr_Digest& operator=(const CryptoMgr_Digest& toBeCopied);
    bool operator==(const CryptoMgr_Digest& toBeCompare) const;
    void SetDigest(unsigned char* data, int len);
    unsigned char* GetDigest() const {
        return (unsigned char*) value;
    }
}__attribute__((__packed__));

class CryptoMgr {
public:
    static inline size_t SizeOfDigest() {
        return sizeof(CryptoMgr_Digest);
    }

    static uint32_t Hash(const CryptoMgr_Input *input);
    static uint32_t Hash(const CryptoMgr_Digest *digest);
    static void ComputeDigest(const CryptoMgr_Input *input, CryptoMgr_Digest &digest);
    static string Tostring(CryptoMgr_Digest *digest);

private:
    CryptoMgr();

};

}
#endif //#ifndef _KV_DB_KEYDIGESTHANDLE_H_

