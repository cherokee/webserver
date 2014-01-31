/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#ifndef CHEROKEE_SHA512
#define CHEROKEE_SHA512

#include "common.h"

#define SHA512_DIGEST_LENGTH 64

struct hc_sha512state {
	unsigned long sz[2];       /* uint64_t */
	unsigned long counter[8];  /* uint64_t */
	unsigned char save[128];
};

typedef struct hc_sha512state CHEROKEE_SHA512_CTX;

void cherokee_SHA512_Init   (CHEROKEE_SHA512_CTX *);
void cherokee_SHA512_Update (CHEROKEE_SHA512_CTX *, const void *, size_t);
void cherokee_SHA512_Final  (CHEROKEE_SHA512_CTX *, void *);

#endif /* CHEROKEE_SHA512 */
