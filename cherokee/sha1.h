#ifndef CHEROKEE_SHA1
#define CHEROKEE_SHA1

#include "common.h"

#define SHA_BLOCKSIZE		64
#define SHA_DIGESTSIZE		20
#define SHA1_DIGEST_SIZE		20

typedef struct {
    unsigned long digest[5];		     /* message digest */
    unsigned long count_lo, count_hi;	/* 64-bit bit count */
    unsigned char data[SHA_BLOCKSIZE];	/* SHA data buffer */
    int local;	   		               /* unprocessed amount in data */
} SHA_INFO;


void sha_init   (SHA_INFO *sha_info);
void sha_update (SHA_INFO *sha_info, unsigned char *buffer, int count);
void sha_final  (SHA_INFO *sha_info, unsigned char digest[20]);

#endif /* CHEROKEE_SHA1 */
