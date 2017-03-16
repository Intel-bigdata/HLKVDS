//  Copyright (c) 2017-present, Intel Corporation.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <stdio.h>
#include <stdlib.h>

#include "KeyDigestHandle.h"

namespace kvdb {

Kvdb_Key::~Kvdb_Key() {
}

Kvdb_Key::Kvdb_Key(const char* key, uint32_t key_len) :
    value(key), len(key_len) {
}

Kvdb_Key::Kvdb_Key(const Kvdb_Key& toBeCopied) {
    len = toBeCopied.len;
    value = toBeCopied.value;
}

Kvdb_Digest::Kvdb_Digest() {
    memset(value, 0, KeyDigestHandle::SizeOfDigest());
}

Kvdb_Digest::Kvdb_Digest(const Kvdb_Digest& toBeCopied) {
    memcpy(value, toBeCopied.value, KeyDigestHandle::SizeOfDigest());
}

Kvdb_Digest::~Kvdb_Digest() {
}

Kvdb_Digest& Kvdb_Digest::operator=(const Kvdb_Digest& toBeCopied) {
    memcpy(value, toBeCopied.value, KeyDigestHandle::SizeOfDigest());
    return *this;
}

bool Kvdb_Digest::operator==(const Kvdb_Digest& toBeCompare) const {
    if (!memcmp(value, toBeCompare.value, KeyDigestHandle::SizeOfDigest())) {
        return true;
    }
    return false;
}

void Kvdb_Digest::SetDigest(unsigned char* data, int len) {
    if ((uint64_t) len != KeyDigestHandle::SizeOfDigest()) {
        return;
    }
    memcpy(value, data, KeyDigestHandle::SizeOfDigest());
}

void KeyDigestHandle::ComputeDigest(const Kvdb_Key *key, Kvdb_Digest &digest)
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

    length = key->GetLen();
    const char* value = key->GetValue();

    /* process message in 16-word chunks */
    for (nbytes = length; nbytes > 63; nbytes -= 64) {
        for (i = 0; i < 16; i++) {
            X[i] = BYTES_TO_DWORD(value);
            value += 4;
        }
        compress(MDbuf, X);
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

uint32_t KeyDigestHandle::Hash(const Kvdb_Key *key) {
    Kvdb_Digest result;
    ComputeDigest(key, result);

    uint32_t hash_value = Hash(&result);

    return hash_value;
}

uint32_t KeyDigestHandle::Hash(const Kvdb_Digest *digest) {
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

string KeyDigestHandle::Tostring(Kvdb_Digest *digest) {
    int digest_size = KeyDigestHandle::SizeOfDigest();
    unsigned char *temp = digest->GetDigest();

    int str_len = 2 * digest_size + 1;
    char *res = new char[str_len];
    for (int i = 0; i < digest_size; i++) {
        sprintf(&res[2 * i], "%02x", temp[i]);
    }
    string result = string((const char*) res);
    delete[] res;
    //delete[] temp;
    return result;
}
}
