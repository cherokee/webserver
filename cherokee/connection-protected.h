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

#ifndef CHEROKEE_CONNECTION_PROTECTED_H
#define CHEROKEE_CONNECTION_PROTECTED_H

#include "common-internal.h"

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#else
# include <time.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif

#include "http.h"
#include "list.h"
#include "avl.h"
#include "socket.h"
#include "header.h"
#include "logger.h"
#include "handler.h"
#include "encoder.h"
#include "iocache.h"
#include "post.h"
#include "header-protected.h"
#include "regex.h"
#include "bind.h"
#include "bogotime.h"
#include "config_entry.h"

typedef enum {
	phase_nothing,
	phase_tls_handshake,
	phase_reading_header,
	phase_processing_header,
	phase_setup_connection,
	phase_init,
	phase_reading_post,
	phase_add_headers,
	phase_send_headers,
	phase_stepping,
	phase_shutdown,
	phase_lingering
} cherokee_connection_phase_t;


#define conn_op_nothing            0
#define conn_op_root_index        (1 << 1)
#define conn_op_tcp_cork          (1 << 2)
#define conn_op_document_root     (1 << 3)
#define conn_op_was_polling       (1 << 4)
#define conn_op_cant_encoder      (1 << 5)
#define conn_op_got_eof           (1 << 6)
#define conn_op_chunked_formatted (1 << 7)

typedef cuint_t cherokee_connection_options_t;


struct cherokee_connection {
	cherokee_list_t               list_node;

	/* References
	 */
	void                         *server;
	void                         *vserver;
	void                         *thread;
	cherokee_bind_t              *bind;
	cherokee_config_entry_ref_t   config_entry;

	/* ID
	 */
	culong_t                      id;
	cherokee_buffer_t             self_trace;

	/* Socket stuff
	 */
	cherokee_socket_t             socket;
	cherokee_http_upgrade_t       upgrade;
	cherokee_connection_options_t options;
	cherokee_handler_t           *handler;

	cherokee_logger_t            *logger_ref;
	cherokee_buffer_t             logger_real_ip;

	/* Buffers
	 */
	cherokee_buffer_t             incoming_header;  /* -> header               */
	cherokee_buffer_t             header_buffer;    /* <- header, -> post data */
	cherokee_buffer_t             buffer;           /* <- data                 */

	/* State
	 */
	cherokee_connection_phase_t   phase;
	cherokee_http_t               error_code;
	cherokee_buffer_t             error_internal_url;
	cherokee_buffer_t             error_internal_qs;
	cherokee_http_t               error_internal_code;

	/* Headers
	 */
	cherokee_header_t             header;

	/* Encoders
	 */
	encoder_func_new_t            encoder_new_func;
	cherokee_encoder_props_t     *encoder_props;
	cherokee_encoder_t           *encoder;
	cherokee_buffer_t             encoder_buffer;

	/* Eg:
	 * http://www.alobbs.com/cherokee/dir/file/param1
	 */
	cherokee_buffer_t             local_directory;     /* /var/www/  or  /home/alo/public_html/ */
	cherokee_buffer_t             web_directory;       /* /cherokee/ */
	cherokee_buffer_t             request;             /* /dir/file */
	cherokee_buffer_t             pathinfo;            /* /param1 */
	cherokee_buffer_t             userdir;             /* 'alo' in http://www.alobbs.com/~alo/thing */
	cherokee_buffer_t             query_string;
	cherokee_avl_t               *arguments;

	cherokee_buffer_t             host;
	cherokee_buffer_t             host_port;
	cherokee_buffer_t             effective_directory;
	cherokee_buffer_t             request_original;
	cherokee_buffer_t             query_string_original;

	/* Authentication
	 */
	cherokee_validator_t         *validator;           /* Validator object */
	cherokee_http_auth_t          auth_type;           /* Auth type of the resource */
	cherokee_http_auth_t          req_auth_type;       /* Auth type of the request  */

	/* Traffic
	 */
	off_t                         rx;                  /* Bytes received */
	size_t                        rx_partial;          /* RX partial counter */
	off_t                         tx;                  /* Bytes sent */
	size_t                        tx_partial;          /* TX partial counter */
	time_t                        traffic_next;        /* Time to update traffic */

	/* Post info
	 */
	cherokee_post_t               post;

	/* Net connection
	 */
	uint32_t                      keepalive;
	time_t                        timeout;
	time_t                        timeout_lapse;
	cherokee_buffer_t            *timeout_header;

	/* Polling
	 */
	int                           polling_fd;
	cherokee_socket_status_t      polling_mode;
	cherokee_boolean_t            polling_multiple;

	off_t                         range_start;
	off_t                         range_end;

	void                         *mmaped;
	off_t                         mmaped_len;
	cherokee_iocache_entry_t     *io_entry_ref;

	/* Front-line cache
	 */
	cherokee_flcache_conn_t       flcache;

	/* Regular expressions
	 */
	int                           regex_ovector[OVECTOR_LEN];
	int                           regex_ovecsize;
	int                           regex_host_ovector[OVECTOR_LEN];
	int                           regex_host_ovecsize;

	/* Content Expiration
	 */
	cherokee_expiration_t         expiration;
	time_t                        expiration_time;
	cherokee_expiration_props_t   expiration_prop;

	/* Chunked encoding
	 */
	cherokee_boolean_t            chunked_encoding;
	cherokee_boolean_t            chunked_last_package;
	cherokee_buffer_t             chunked_len;
	size_t                        chunked_sent;
	struct iovec                  chunks[3];
	uint16_t                      chunksn;

	/* Redirections
	 */
	cherokee_buffer_t             redirect;
	cuint_t                       respins;

	/* Traffic-shaping
	 */
	cherokee_boolean_t            limit_rate;
	cuint_t                       limit_bps;
	cherokee_msec_t               limit_blocked_until;
};

#define CONN_SRV(c)    (SRV(CONN(c)->server))
#define CONN_HDR(c)    (HDR(CONN(c)->header))
#define CONN_SOCK(c)   (SOCKET(CONN(c)->socket))
#define CONN_VSRV(c)   (VSERVER(CONN(c)->vserver))
#define CONN_THREAD(c) (THREAD(CONN(c)->thread))
#define CONN_BIND(c)   (BIND(CONN(c)->bind))

#define TRACE_CONN(c)  TRACE("conn", "%s", cherokee_connection_print(c));

/* Basic functions
 */
ret_t cherokee_connection_new                    (cherokee_connection_t **conn);
ret_t cherokee_connection_free                   (cherokee_connection_t  *conn);
ret_t cherokee_connection_clean                  (cherokee_connection_t  *conn);
ret_t cherokee_connection_clean_close            (cherokee_connection_t  *conn);

/* Close
 */
ret_t cherokee_connection_shutdown_wr            (cherokee_connection_t *conn);
ret_t cherokee_connection_linger_read            (cherokee_connection_t *conn);

/* Connection I/O
 */
ret_t cherokee_connection_send                   (cherokee_connection_t *conn);
ret_t cherokee_connection_send_header            (cherokee_connection_t *conn);
ret_t cherokee_connection_send_header_and_mmaped (cherokee_connection_t *conn);
ret_t cherokee_connection_recv                   (cherokee_connection_t *conn, cherokee_buffer_t *buffer, off_t to_read, off_t *len);

/* Internal
 */
ret_t cherokee_connection_create_handler         (cherokee_connection_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_connection_create_encoder         (cherokee_connection_t *conn, cherokee_avl_t *accept_enc);
ret_t cherokee_connection_setup_error_handler    (cherokee_connection_t *conn);
ret_t cherokee_connection_setup_hsts_handler     (cherokee_connection_t *conn);
ret_t cherokee_connection_check_authentication   (cherokee_connection_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_connection_check_ip_validation    (cherokee_connection_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_connection_check_only_secure      (cherokee_connection_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_connection_check_http_method      (cherokee_connection_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_connection_set_rate               (cherokee_connection_t *conn, cherokee_config_entry_t *config_entry);
void  cherokee_connection_set_keepalive          (cherokee_connection_t *conn);
void  cherokee_connection_set_chunked_encoding   (cherokee_connection_t *conn);
int   cherokee_connection_should_include_length  (cherokee_connection_t *conn);
ret_t cherokee_connection_instance_encoder       (cherokee_connection_t *conn);
ret_t cherokee_connection_sleep                  (cherokee_connection_t *conn, cherokee_msec_t msecs);
void  cherokee_connection_update_timeout         (cherokee_connection_t *conn);
void  cherokee_connection_add_expiration_header  (cherokee_connection_t *conn, cherokee_buffer_t *buffer, cherokee_boolean_t use_maxage);
ret_t cherokee_connection_build_host_string      (cherokee_connection_t *conn, cherokee_buffer_t *buf);
ret_t cherokee_connection_build_host_port_string (cherokee_connection_t *conn, cherokee_buffer_t *buf);

/* Iteration
 */
ret_t cherokee_connection_open_request           (cherokee_connection_t *conn);
ret_t cherokee_connection_reading_check          (cherokee_connection_t *conn);
ret_t cherokee_connection_step                   (cherokee_connection_t *conn);

/* Headers
 */
ret_t cherokee_connection_read_post              (cherokee_connection_t *conn);
ret_t cherokee_connection_build_header           (cherokee_connection_t *conn);
ret_t cherokee_connection_get_request            (cherokee_connection_t *conn);
ret_t cherokee_connection_parse_range            (cherokee_connection_t *conn);
int   cherokee_connection_is_userdir             (cherokee_connection_t *conn);

ret_t cherokee_connection_build_local_directory  (cherokee_connection_t *conn, cherokee_virtual_server_t *vsrv);
ret_t cherokee_connection_build_local_directory_userdir (cherokee_connection_t *conn, cherokee_virtual_server_t *vsrv);
ret_t cherokee_connection_set_custom_droot       (cherokee_connection_t *conn, cherokee_config_entry_t *entry);

ret_t cherokee_connection_clean_error_headers    (cherokee_connection_t *conn);
ret_t cherokee_connection_set_redirect           (cherokee_connection_t *conn, cherokee_buffer_t *address);

ret_t cherokee_connection_clean_for_respin       (cherokee_connection_t *conn);
int   cherokee_connection_use_webdir             (cherokee_connection_t *conn);
void  cherokee_connection_set_pathinfo           (cherokee_connection_t *conn);

/* Log
 */
ret_t cherokee_connection_log                    (cherokee_connection_t *conn);
ret_t cherokee_connection_update_vhost_traffic   (cherokee_connection_t *conn);
char *cherokee_connection_print                  (cherokee_connection_t *conn);

/* Transfers
 */
void cherokee_connection_rx_add                  (cherokee_connection_t *conn, ssize_t rx);
void cherokee_connection_tx_add                  (cherokee_connection_t *conn, ssize_t tx);

#endif /* CHEROKEE_CONNECTION_PROTECTED_H */
