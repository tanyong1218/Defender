/*
 * Copyright 1995-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#ifndef _SHA1_H__X_
#define _SHA1_H__X_
#include <stdlib.h>
#include <string.h>



#define DATA_ORDER_IS_BIG_ENDIAN
#define HASH_LONG               SHA_LONG
#define HASH_CTX                SHA_CTX
#define HASH_CBLOCK             SHA_CBLOCK
#define HASH_MAKE_STRING(c,s)   do {    \
        unsigned long ll;               \
        ll=(c)->h0; (void)HOST_l2c(ll,(s));     \
        ll=(c)->h1; (void)HOST_l2c(ll,(s));     \
        ll=(c)->h2; (void)HOST_l2c(ll,(s));     \
        ll=(c)->h3; (void)HOST_l2c(ll,(s));     \
        ll=(c)->h4; (void)HOST_l2c(ll,(s));     \
        } while (0)

#define HASH_UPDATE                     SHA1_Update
#define HASH_TRANSFORM                  SHA1_Transform
#define HASH_FINAL                      SHA1_Final
#define HASH_INIT                       SHA1_Init
#define HASH_BLOCK_DATA_ORDER           sha1_block_data_order
#define Xupdate(a,ix,ia,ib,ic,id)       ( (a)=(ia^ib^ic^id),    \
                                          ix=(a)=ROTATE((a),1)  \
                                        )
/*-
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * ! SHA_LONG has to be at least 32 bits wide.                    !
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */
#  define SHA_LONG unsigned int

#  define SHA_LBLOCK      16
#  define SHA_CBLOCK      (SHA_LBLOCK*4)/* SHA treats input data as a
                                         * contiguous array of 32 bit wide
                                         * big-endian values. */
#  define SHA_LAST_BLOCK  (SHA_CBLOCK-8)

typedef struct SHAstate_st {
    SHA_LONG h0, h1, h2, h3, h4;
    SHA_LONG Nl, Nh;
    SHA_LONG data[SHA_LBLOCK];
    unsigned int num;
} SHA_CTX;


#ifdef __cplusplus
extern "C" {
#endif
	int SHA1_Init(SHA_CTX *c);
	void SHA1_Transform(HASH_CTX *c, const unsigned char *data);
	int SHA1_Update(HASH_CTX *c, const void *data_, size_t len);
	int SHA1_Final(unsigned char *md, HASH_CTX *c);




#ifdef __cplusplus
}
#endif




#endif //_SHA1_H__X_