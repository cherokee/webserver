#ifndef CHEROKEE_CRYPTOR_LIBSSL_COMPAT_H
#define CHEROKEE_CRYPTOR_LIBSSL_COMPAT_H

/* OpenSSL 3.0 made DH an opaque provider-backed key type. */
#if OPENSSL_VERSION_NUMBER >= 0x30000000L

# include <openssl/core_names.h>
# include <openssl/param_build.h>
# include <openssl/evp.h>

typedef EVP_PKEY cherokee_dh_param_t;

static cherokee_dh_param_t *
cherokee_dh_param_new (const unsigned char *p, size_t p_len,
                       const unsigned char *g, size_t g_len)
{
	EVP_PKEY_CTX  *ctx = NULL;
	EVP_PKEY      *key = NULL;
	OSSL_PARAM_BLD *builder = NULL;
	OSSL_PARAM     *params = NULL;
	BIGNUM         *p_bn = NULL;
	BIGNUM         *g_bn = NULL;

	p_bn = BN_bin2bn (p, p_len, NULL);
	g_bn = BN_bin2bn (g, g_len, NULL);
	builder = OSSL_PARAM_BLD_new ();
	if (p_bn == NULL || g_bn == NULL || builder == NULL ||
	    OSSL_PARAM_BLD_push_BN (builder, OSSL_PKEY_PARAM_FFC_P, p_bn) != 1 ||
	    OSSL_PARAM_BLD_push_BN (builder, OSSL_PKEY_PARAM_FFC_G, g_bn) != 1)
		goto out;

	params = OSSL_PARAM_BLD_to_param (builder);
	ctx = EVP_PKEY_CTX_new_from_name (NULL, "DH", NULL);
	if (params == NULL || ctx == NULL || EVP_PKEY_fromdata_init (ctx) <= 0 ||
	    EVP_PKEY_fromdata (ctx, &key, EVP_PKEY_KEY_PARAMETERS, params) <= 0)
		key = NULL;

out:
	EVP_PKEY_CTX_free (ctx);
	OSSL_PARAM_free (params);
	OSSL_PARAM_BLD_free (builder);
	BN_free (p_bn);
	BN_free (g_bn);
	return key;
}

# define cherokee_dh_param_free EVP_PKEY_free
# define cherokee_dh_param_up_ref EVP_PKEY_up_ref

#else

# include <openssl/dh.h>

typedef DH cherokee_dh_param_t;

static cherokee_dh_param_t *
cherokee_dh_param_new (const unsigned char *p, size_t p_len,
                       const unsigned char *g, size_t g_len)
{
	DH     *dh;
	BIGNUM *p_bn;
	BIGNUM *g_bn;

	dh = DH_new ();
	if (dh == NULL)
		return NULL;
	p_bn = BN_bin2bn (p, p_len, NULL);
	g_bn = BN_bin2bn (g, g_len, NULL);
	if (p_bn == NULL || g_bn == NULL) {
		DH_free (dh);
		BN_free (p_bn);
		BN_free (g_bn);
		return NULL;
	}

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	dh->p = p_bn;
	dh->g = g_bn;
#else
	if (!DH_set0_pqg (dh, p_bn, NULL, g_bn)) {
		DH_free (dh);
		BN_free (p_bn);
		BN_free (g_bn);
		return NULL;
	}
#endif
	return dh;
}

# define cherokee_dh_param_free DH_free

#endif

#endif
