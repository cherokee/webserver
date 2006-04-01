/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef CHEROKEE_VIRTUAL_SERVER_H
#define CHEROKEE_VIRTUAL_SERVER_H

#include "common-internal.h"

#ifdef HAVE_GNUTLS
# include <gnutls/extra.h>
# include <gnutls/gnutls.h>
#endif

#ifdef HAVE_OPENSSL
# include <openssl/ssl.h>
#endif

#include "list.h"
#include "session_cache.h"
#include "handler.h"
#include "dirs_table.h"
#include "exts_table.h"
#include "reqs_list.h"
#include "config_entry.h"
#include "logger.h"


typedef struct {
	struct list_head list_entry;

	cherokee_buffer_t           *name;            /* Default name.   Eg: www.0x50.org */

	cherokee_dirs_table_t        dirs;            /* Eg: (/public, common) */
	cherokee_exts_table_t       *exts;            /* Eg: (.php,    phpcgi) */
	cherokee_reqs_list_t         reqs;            /* Eg: ("*.mp3"  auth{}) */

	cherokee_config_entry_t     *default_handler; /* Default handler */
	cherokee_config_entry_t     *error_handler;   /* Default internal error handler   */

	cherokee_logger_t           *logger;          /* Logger obj              */
	cherokee_table_t            *logger_props;    /* Logger properties table */

	cherokee_buffer_t           *root;            /* Document root. Eg: /var/www */
	cherokee_buffer_t           *userdir;         /* Eg: public_html             */
	cherokee_dirs_table_t       *userdir_dirs;    /* Eg: (/public, common)       */

	list_t                       index_list;      /* Eg: index.html, index.php  */
	cherokee_boolean_t           relative_paths;  /* Allow ".." in the requests */

	struct {                                      /* Number of bytes {up,down}loaded */
		size_t tx;
		size_t rx;

#ifdef HAVE_PTHREAD
		pthread_mutex_t tx_mutex;
		pthread_mutex_t rx_mutex;
#endif
	} data;

	char *server_cert;
	char *server_key;
	char *ca_cert;

#ifdef HAVE_TLS
	cherokee_session_cache_t *session_cache;

# ifdef HAVE_GNUTLS
	gnutls_certificate_server_credentials credentials;
	gnutls_dh_params                      dh_params;
	gnutls_rsa_params                     rsa_params;
# endif
# ifdef HAVE_OPENSSL
	SSL_CTX *context;
# endif
#endif

} cherokee_virtual_server_t;

#define VSERVER(v)        ((cherokee_virtual_server_t *)(v))
#define VSERVER_LOGGER(v) (LOGGER(VSERVER(v)->logger))


ret_t cherokee_virtual_server_new   (cherokee_virtual_server_t **vserver);
ret_t cherokee_virtual_server_free  (cherokee_virtual_server_t  *vserver);
ret_t cherokee_virtual_server_clean (cherokee_virtual_server_t  *vserver);

ret_t cherokee_virtual_server_init_tls (cherokee_virtual_server_t *vserver);
ret_t cherokee_virtual_server_have_tls (cherokee_virtual_server_t *vserver);

void  cherokee_virtual_server_add_rx (cherokee_virtual_server_t *vserver, size_t rx);
void  cherokee_virtual_server_add_tx (cherokee_virtual_server_t *vserver, size_t tx);

#endif /* CHEROKEE_VIRTUAL_SERVER_H */
