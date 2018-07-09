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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_CRYPTOR_H
#define CHEROKEE_CRYPTOR_H

#include <cherokee/common.h>
#include <cherokee/buffer.h>
#include <cherokee/module.h>

CHEROKEE_BEGIN_DECLS

#define CHEROKEE_CIPHERS_DEFAULT "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA"

/* Callback function prototipes
 */
typedef ret_t (* cryptor_func_new_t)         (void **cryp);
typedef ret_t (* cryptor_func_free_t)        (void  *cryp);
typedef ret_t (* cryptor_func_configure_t)   (void  *cryp, cherokee_config_node_t *, cherokee_server_t *);
typedef ret_t (* cryptor_func_vserver_new_t) (void  *cryp, void *vsrv, void **vserver_crypt);
typedef ret_t (* cryptor_func_socket_new_t)  (void  *cryp, void **socket_crypt);
typedef ret_t (* cryptor_func_client_new_t)  (void  *cryp, void **client_crypt);

/* Cryptor: Virtual server */
typedef ret_t (* cryptor_vsrv_func_free_t)      (void  *cryp);

/* Cryptor: Socket */
typedef ret_t (* cryptor_socket_func_free_t)    (void *cryp);
typedef ret_t (* cryptor_socket_func_clean_t)   (void *cryp);
typedef ret_t (* cryptor_socket_func_init_tls_t)(void *cryp, void *sock, void *vsrv, void *conn, void *blocking);
typedef ret_t (* cryptor_socket_func_shutdown_t)(void *cryp);
typedef ret_t (* cryptor_socket_func_read_t)    (void *cryp, char *buf, int len, size_t *re_len);
typedef ret_t (* cryptor_socket_func_write_t)   (void *cryp, char *buf, int len, size_t *re_len);
typedef int   (* cryptor_socket_func_pending_t) (void *cryp);

/* Cryptor: Client socket */
typedef ret_t (* cryptor_client_func_init_t)    (void  *cryp, void *host, void *socket);

/* Data types
 */
typedef struct {
	cherokee_module_t          module;
	cint_t                     timeout_handshake;
	cherokee_boolean_t         allow_SSLv2;
	cherokee_boolean_t         allow_SSLv3;
	cherokee_boolean_t         allow_TLSv1;
	cherokee_boolean_t         allow_TLSv1_1;
	cherokee_boolean_t         allow_TLSv1_2;

	/* Methods */
	cryptor_func_configure_t   configure;
	cryptor_func_vserver_new_t vserver_new;
	cryptor_func_socket_new_t  socket_new;
	cryptor_func_client_new_t  client_new;
} cherokee_cryptor_t;

typedef struct {
	/* Methods */
	cryptor_vsrv_func_free_t   free;
} cherokee_cryptor_vserver_t;

typedef struct {
	cherokee_boolean_t             initialized;
	void                          *vserver_ref;

	/* Methods */
	cryptor_socket_func_free_t     free;
	cryptor_socket_func_clean_t    clean;
	cryptor_socket_func_init_tls_t init_tls;
	cryptor_socket_func_shutdown_t shutdown;
	cryptor_socket_func_read_t     read;
	cryptor_socket_func_write_t    write;
	cryptor_socket_func_pending_t  pending;
} cherokee_cryptor_socket_t;

typedef struct {
	cherokee_cryptor_socket_t      base;
} cherokee_cryptor_client_t;


#define CRYPTOR(x)        ((cherokee_cryptor_t *)(x))
#define CRYPTOR_VSRV(x)   ((cherokee_cryptor_vserver_t *)(x))
#define CRYPTOR_SOCKET(x) ((cherokee_cryptor_socket_t *)(x))
#define CRYPTOR_CLIENT(x) ((cherokee_cryptor_client_t *)(x))

/* Easy initialization
 */
#define CRYPTOR_CONF_PROTOTYPE(name)                                \
	ret_t cherokee_cryptor_ ## name ## _configure (             \
	        cherokee_config_node_t   *,                         \
	        cherokee_server_t        *,                         \
	        cherokee_module_props_t **)

#define PLUGIN_INFO_CRYPTOR_EASY_INIT(name)                         \
	CRYPTOR_CONF_PROTOTYPE(name);                               \
	                                                            \
	PLUGIN_INFO_INIT(name, cherokee_cryptor,                    \
	        (void *)cherokee_cryptor_ ## name ## _new,          \
	        (void *)NULL)

#define PLUGIN_INFO_CRYPTOR_EASIEST_INIT(name)                      \
	PLUGIN_EMPTY_INIT_FUNCTION(name)                            \
	PLUGIN_INFO_CRYPTOR_EASY_INIT(name)


/* Cryptor: Server
 */
ret_t cherokee_cryptor_init_base   (cherokee_cryptor_t          *cryp,
                                    cherokee_plugin_info_t      *info);
ret_t cherokee_cryptor_free_base   (cherokee_cryptor_t          *cryp);
ret_t cherokee_cryptor_free        (cherokee_cryptor_t          *cryp);

ret_t cherokee_cryptor_configure   (cherokee_cryptor_t          *cryp,
                                    cherokee_config_node_t      *conf,
                                    cherokee_server_t           *srv);

ret_t cherokee_cryptor_vserver_new (cherokee_cryptor_t          *cryp,
                                    void                        *vsrv,
                                    cherokee_cryptor_vserver_t **cryp_vsrv);

ret_t cherokee_cryptor_socket_new  (cherokee_cryptor_t          *cryp,
                                    cherokee_cryptor_socket_t  **cryp_sock);

ret_t cherokee_cryptor_client_new  (cherokee_cryptor_t         *cryp,
                                    cherokee_cryptor_client_t **cryp_client);

/* Cryptor: Virtual Server
 */
ret_t cherokee_cryptor_vserver_init_base (cherokee_cryptor_vserver_t *cryp);
ret_t cherokee_cryptor_vserver_free      (cherokee_cryptor_vserver_t *cryp);

/* Cryptor: Socket
 */
ret_t cherokee_cryptor_socket_init_base   (cherokee_cryptor_socket_t *cryp);
ret_t cherokee_cryptor_socket_clean_base  (cherokee_cryptor_socket_t *cryp);

ret_t cherokee_cryptor_socket_free        (cherokee_cryptor_socket_t *cryp);
ret_t cherokee_cryptor_socket_clean       (cherokee_cryptor_socket_t *cryp);
ret_t cherokee_cryptor_socket_shutdown    (cherokee_cryptor_socket_t *cryp);
ret_t cherokee_cryptor_socket_init_tls    (cherokee_cryptor_socket_t *cryp,
                                           void                      *sock,
                                           void                      *vsrv,
                                           void                      *conn,
                                           void                      *blocking);
ret_t cherokee_cryptor_socket_read        (cherokee_cryptor_socket_t *cryp,
                                           char                      *buf,
                                           int                        buf_size,
                                           size_t                    *pcnt_read);
ret_t cherokee_cryptor_socket_write       (cherokee_cryptor_socket_t *cryp,
                                           char                      *buf,
                                           int                        buf_len,
                                           size_t                    *written);
int   cherokee_cryptor_socket_pending     (cherokee_cryptor_socket_t *cryp);

/* Cryptor: Client Socket
 */
ret_t cherokee_cryptor_client_init        (cherokee_cryptor_client_t *cryp,
                                           cherokee_buffer_t         *host,
                                           void                      *socket);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_CRYPTOR_H */
