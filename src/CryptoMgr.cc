#include <stdio.h>
#include <stdlib.h>
#include "CryptoMgr.h"
namespace dslab{
//-------------------------input------------------------------
CryptoMgr_Input::~CryptoMgr_Input() {
}

CryptoMgr_Input::CryptoMgr_Input(const char* input, uint32_t input_len) :
    value(input), len(input_len) {
}

CryptoMgr_Input::CryptoMgr_Input(const CryptoMgr_Input& toBeCopied) {
    len = toBeCopied.len;
    value = toBeCopied.value;
}
//------------------------digest-----------------------
CryptoMgr_Digest::CryptoMgr_Digest() {
    memset(value, 0, CryptoMgr::SizeOfDigest());
}

CryptoMgr_Digest::CryptoMgr_Digest(const CryptoMgr_Digest& toBeCopied) {
    memcpy(value, toBeCopied.value, CryptoMgr::SizeOfDigest());
}

CryptoMgr_Digest::~CryptoMgr_Digest() {
}

CryptoMgr_Digest& CryptoMgr_Digest::operator=(const CryptoMgr_Digest& toBeCopied) {
    memcpy(value, toBeCopied.value, CryptoMgr::SizeOfDigest());
    return *this;
}

bool CryptoMgr_Digest::operator==(const CryptoMgr_Digest& toBeCompare) const {
    if (!memcmp(value, toBeCompare.value, CryptoMgr::SizeOfDigest())) {
        return true;
    }
    return false;
}

void CryptoMgr_Digest::SetDigest(unsigned char* data, int len) {
    if ((uint64_t) len != CryptoMgr::SizeOfDigest()) {
        return;
    }
    memcpy(value, data, CryptoMgr::SizeOfDigest());
}
//--------------------------------CryptoMgr---------------------------------
void CryptoMgr::ComputeDigest(const CryptoMgr_Input *input, CryptoMgr_Digest &digest)
/*
 * returns RMD(message)
 * message should be a string terminated by '\0'
 */
{
    dword MDbuf[RMDsize / 32]; /* contains (A, B, C, D(, E))   */
    //static byte   hashcode[RMDsize/8]; /* for final hash-value         */
    byte hashcode[RMDsize / 8]; /* for final hash-value         */
    dword X[16]; /* current 16-word chunk        */
    unsigned int i; /* counter                      */
    dword length; /* length in bytes of message   */
    dword nbytes; /* # of bytes not yet processed */

    /* initialize */
    MDinit(MDbuf);

    length = input->GetLen();
    const char* value = input->GetValue();

    /* process message in 16-word chunks */
    for (nbytes = length; nbytes > 63; nbytes -= 64) {
        for (i = 0; i < 16; i++) {
            X[i] = BYTES_TO_DWORD(value);
            value += 4;
        }
        compress_(MDbuf, X);
    } /* length mod 64 bytes left */

    /* finish: */
    MDfinish(MDbuf, (byte *) value, length, 0);

    for (i = 0; i < RMDsize / 8; i += 4) {
        hashcode[i] = MDbuf[i >> 2]; /* implicit cast to byte  */
        hashcode[i + 1] = (MDbuf[i >> 2] >> 8); /*  extracts the 8 least  */
        hashcode[i + 2] = (MDbuf[i >> 2] >> 16); /*  significant bits.     */
        hashcode[i + 3] = (MDbuf[i >> 2] >> 24);
    }

    digest.SetDigest((unsigned char*) hashcode, RMDsize / 8);
}


uint32_t CryptoMgr::Hash(const CryptoMgr_Input *input) {
    CryptoMgr_Digest result;
    ComputeDigest(input, result);

    uint32_t hash_value = Hash(&result);

    return hash_value;
}

uint32_t CryptoMgr::Hash(const CryptoMgr_Digest *digest) {
    uint32_t hash_value;
    const uint32_t *pi = &digest->value[4];
    unsigned char key_char[4];
    key_char[0] = *((const char *) pi + 0);
    key_char[1] = *((const char *) pi + 1);
    key_char[2] = *((const char *) pi + 2);
    key_char[3] = *((const char *) pi + 3);
    hash_value = (key_char[0]) + (key_char[1] << 8) + (key_char[2] << 16)
            + (key_char[3] << 24);
    return hash_value;
}

string CryptoMgr::Tostring(CryptoMgr_Digest *digest) {
    int digest_size = CryptoMgr::SizeOfDigest();
    unsigned char *temp = digest->GetDigest();

    int str_len = 2 * digest_size + 1;
    char *res = new char[str_len];
    for (int i = 0; i < digest_size; i++) {
        sprintf(&res[2 * i], "%02x", temp[i]);
    }
    string result = string((const char*) res);
    delete[] res;
    return result;
}

}
