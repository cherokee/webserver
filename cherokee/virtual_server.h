/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2008 Alvaro Lopez Ortega
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
#include <unistd.h>

#ifdef HAVE_GNUTLS
# include <gnutls/extra.h>
# include <gnutls/gnutls.h>
# include <gnutls/x509.h>
#endif

#ifdef HAVE_OPENSSL
# include <openssl/ssl.h>
#endif

#include "avl.h"
#include "avl_r.h"
#include "list.h"
#include "handler.h"
#include "config_entry.h"
#include "logger.h"
#include "config_node.h"
#include "virtual_server_names.h"
#include "rule_list.h"

typedef struct {
	cherokee_list_t              list_node;
	void                        *server_ref;      /* Ref to server */

	cherokee_buffer_t            name;            /* Name.    Eg: server1        */
	cuint_t                      priority;        /* Evaluation priority         */
	cherokee_vserver_names_t     domains;         /* Domains. Eg: www.alobbs.com */
	cherokee_rule_list_t         rules;           /* Rule list: vserver behavior */

	cherokee_config_entry_t     *default_handler; /* Default handler             */
	cherokee_config_entry_t     *error_handler;   /* Default error handler       */

	cherokee_logger_t           *logger;          /* Logger object               */
	cherokee_avl_t              *logger_props;    /* Logger properties table     */

	cherokee_buffer_t            userdir;         /* Eg: public_html             */
	cherokee_rule_list_t         userdir_rules;   /* User dir behavior           */

	cherokee_buffer_t            root;            /* Document root. Eg: /var/www */
	cherokee_list_t              index_list;      /* Eg: index.html, index.php   */

	struct {                                      /* Number of bytes {up,down}loaded */
		off_t                tx;
		off_t                rx;
		CHEROKEE_MUTEX_T    (tx_mutex);
		CHEROKEE_MUTEX_T    (rx_mutex);
	} data;

	cherokee_buffer_t            server_cert;
	cherokee_buffer_t            server_key;
	cherokee_buffer_t            ca_cert;

#ifdef HAVE_TLS
	cherokee_avl_r_t             session_cache;

# ifdef HAVE_GNUTLS   
	gnutls_certificate_server_credentials credentials;
	gnutls_x509_privkey_t                 privkey_x509;
	gnutls_x509_crt_t                     certs_x509;

	gnutls_dh_params             dh_params;
	gnutls_rsa_params            rsa_params;
# endif
# ifdef HAVE_OPENSSL
	SSL_CTX                     *context;
# endif
#endif

} cherokee_virtual_server_t;

#define VSERVER(v)        ((cherokee_virtual_server_t *)(v))
#define VSERVER_LOGGER(v) (LOGGER(VSERVER(v)->logger))
#define VSERVER_SRV(v)    (SRV(VSERVER(v)->server_ref))

ret_t cherokee_virtual_server_new       (cherokee_virtual_server_t **vserver, void *server);
ret_t cherokee_virtual_server_free      (cherokee_virtual_server_t  *vserver);
ret_t cherokee_virtual_server_clean     (cherokee_virtual_server_t  *vserver);

ret_t cherokee_virtual_server_configure (cherokee_virtual_server_t  *vserver, 
					 cuint_t                     prio,
					 cherokee_config_node_t     *config);

ret_t cherokee_virtual_server_new_rule  (cherokee_virtual_server_t  *vserver, 
					 cherokee_config_node_t     *config, 
					 cuint_t                     priority,
					 cherokee_rule_t           **rule);

ret_t cherokee_virtual_server_init_tls  (cherokee_virtual_server_t *vserver);
ret_t cherokee_virtual_server_has_tls   (cherokee_virtual_server_t *vserver);

void  cherokee_virtual_server_add_rx    (cherokee_virtual_server_t *vserver, size_t rx);
void  cherokee_virtual_server_add_tx    (cherokee_virtual_server_t *vserver, size_t tx);

#endif /* CHEROKEE_VIRTUAL_SERVER_H */
