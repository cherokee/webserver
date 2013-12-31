/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2014 Alvaro Lopez Ortega
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/* IN CASE THIS PLUG-IN IS COMPILED WITH OPENSSL:
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU General Public License in all respects for
 * all of the code used other than OpenSSL.  If you modify file(s)
 * with this exception, you may extend this exception to your version
 * of the file(s), but you are not obligated to do so.  If you do not
 * wish to do so, delete this exception statement from your version.
 * If you delete this exception statement from all source files in the
 * program, then also delete it here.
 */

#ifndef CHEROKEE_CRYPTOR_LIBSSL_H
#define CHEROKEE_CRYPTOR_LIBSSL_H

#include "common.h"
#include "avl_r.h"
#include "module.h"
#include "cryptor.h"
#include "plugin_loader.h"

#include <openssl/lhash.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/conf.h>

#if HAVE_OPENSSL_ENGINE_H
# include <openssl/engine.h>
#endif

#ifndef OPENSSL_NO_TLSEXT
# ifndef SSL_CTRL_SET_TLSEXT_HOSTNAME
#  define OPENSSL_NO_TLSEXT
# endif
#endif

/* Data types
 */
typedef struct {
	cherokee_cryptor_t          base;
	int                         foo;
} cherokee_cryptor_libssl_t;

typedef struct {
	cherokee_cryptor_vserver_t  base;
	SSL_CTX                    *context;
} cherokee_cryptor_vserver_libssl_t;

typedef struct {
	cherokee_cryptor_socket_t  base;
	SSL                       *session;
	SSL_CTX                   *ssl_ctx;
	cherokee_boolean_t         is_pending;
	struct {
		char              *buf;
		off_t              buf_len;
		off_t              written;
	} writing;
} cherokee_cryptor_socket_libssl_t;

typedef struct {
	cherokee_cryptor_client_t  base;
	SSL                       *session;
	SSL_CTX                   *ssl_ctx;
} cherokee_cryptor_client_libssl_t;

#define OPENSSL_LAST_ERROR(error)                          \
	do {                                               \
		int n;                                     \
		error = "unknown";                         \
		while ((n = ERR_get_error()))              \
			error = ERR_error_string(n, NULL); \
	} while (0)

#define CRYPTOR_SSL(x)      ((cherokee_cryptor_libssl_t *)(x))
#define CRYPTOR_VSRV_SSL(x) ((cherokee_cryptor_vserver_libssl_t *)(x))
#define CRYPTOR_SOCK_SSL(x) ((cherokee_cryptor_socket_libssl_t *)(x))

/* Methods
 */
void PLUGIN_INIT_NAME(libssl) (cherokee_plugin_loader_t *loader);

ret_t cherokee_cryptor_libssl_new (cherokee_cryptor_libssl_t **encoder);

#endif /* CHEROKEE_CRYPTOR_LIBSSL_H */
