#if OPENSSL_VERSION_NUMBER < 0x10100000L
#include <string.h>
#include <openssl/engine.h>

int DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g)
{
	/* If the fields p and g in d are NULL, the corresponding input
	 * parameters MUST be non-NULL.  q may remain NULL.
	 */

	if ((dh->p == NULL && p == NULL)
	    || (dh->g == NULL && g == NULL))
		return 0;

	if (p != NULL) {
		BN_free(dh->p);
		dh->p = p;
	}

	if (q != NULL) {
		BN_free(dh->q);
		dh->q = q;
	}

	if (g != NULL) {
		BN_free(dh->g);
		dh->g = g;
	}

	if (q != NULL) {
		dh->length = BN_num_bits(q);
	}

	return 1;
}
#endif
