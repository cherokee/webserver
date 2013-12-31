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

#ifndef CHEROKEE_VIRTUAL_SERVER_H
#define CHEROKEE_VIRTUAL_SERVER_H

#include "common-internal.h"
#include <unistd.h>

#include "avl.h"
#include "list.h"
#include "handler.h"
#include "config_entry.h"
#include "logger.h"
#include "config_node.h"
#include "rule_list.h"
#include "cryptor.h"
#include "vrule.h"
#include "gen_evhost.h"
#include "collector.h"
#include "flcache.h"

typedef enum {
	req_client_cert_skip = 0,
	req_client_cert_tolerate,
	req_client_cert_accept,
	req_client_cert_require
} cherokee_req_client_cert_t;

typedef struct {
	cherokee_list_t              list_node;
	void                        *server_ref;      /* Ref to server */

	cherokee_buffer_t            name;            /* Name.    Eg: server1        */
	cuint_t                      priority;        /* Evaluation priority         */
	cherokee_vrule_t            *matching;        /* Matching rule               */
	cherokee_boolean_t           match_nick;      /* Match nickname as well      */

	cherokee_rule_list_t         rules;           /* Rule list: vserver behavior */
	cherokee_boolean_t           keepalive;       /* Keep-alive support          */
	cint_t                       post_max_len;    /* Max post length             */

	cherokee_config_entry_t     *default_handler; /* Default handler             */
	cherokee_config_entry_t     *error_handler;   /* Default error handler       */

	cherokee_logger_t           *logger;          /* Logger object               */
	cherokee_logger_writer_t    *error_writer;    /* Error writer object         */
	cherokee_collector_vsrv_t   *collector;       /* Information collector       */

	cherokee_buffer_t            userdir;         /* Eg: public_html             */
	cherokee_rule_list_t         userdir_rules;   /* User dir behavior           */

	cherokee_buffer_t            root;            /* Document root. Eg: /var/www */
	cherokee_list_t              index_list;      /* Eg: index.html, index.php   */
	cherokee_flcache_t          *flcache;         /* Front Line cache            */
	void                        *evhost;

	cuint_t                      verify_depth;
	cherokee_buffer_t            server_cert;
	cherokee_buffer_t            server_key;
	cherokee_buffer_t            certs_ca;
	cherokee_req_client_cert_t   req_client_certs;
	cherokee_buffer_t            ciphers;
	cherokee_boolean_t           cipher_server_preference;
	cherokee_boolean_t           ssl_compression;
	cuint_t                      ssl_dh_length;
	cherokee_cryptor_vserver_t  *cryptor;

	struct {
		cherokee_boolean_t   enabled;
		cherokee_boolean_t   subdomains;
		cuint_t              max_age;
	} hsts;

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

ret_t cherokee_virtual_server_new_vrule (cherokee_virtual_server_t  *vserver,
                                         cherokee_config_node_t     *config,
                                         cherokee_vrule_t          **vrule);

ret_t cherokee_virtual_server_init_tls  (cherokee_virtual_server_t *vserver);
ret_t cherokee_virtual_server_has_tls   (cherokee_virtual_server_t *vserver);

void  cherokee_virtual_server_add_rx    (cherokee_virtual_server_t *vserver, size_t rx);
void  cherokee_virtual_server_add_tx    (cherokee_virtual_server_t *vserver, size_t tx);

ret_t cherokee_virtual_server_get_error_log (cherokee_virtual_server_t  *vserver,
                                             cherokee_logger_writer_t  **writer);

#endif /* CHEROKEE_VIRTUAL_SERVER_H */
