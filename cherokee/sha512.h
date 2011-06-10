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

typedef struct hc_sha512state SHA512_CTX;

void SHA512_Init   (SHA512_CTX *);
void SHA512_Update (SHA512_CTX *, const void *, size_t);
void SHA512_Final  (SHA512_CTX *, void *);

#endif /* CHEROKEE_SHA512 */
