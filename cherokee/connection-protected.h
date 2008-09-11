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

typedef enum {
	phase_nothing,
	phase_switching_headers,
	phase_tls_handshake,
	phase_reading_header,
	phase_processing_header,
	phase_read_post,
	phase_setup_connection,
	phase_init,
	phase_add_headers,
	phase_send_headers,
	phase_steping,
	phase_shutdown,
	phase_lingering
} cherokee_connection_phase_t;


#define conn_op_nothing        0
#define conn_op_log_at_end    (1 << 0)
#define conn_op_root_index    (1 << 1)
#define conn_op_tcp_cork      (1 << 2)
#define conn_op_document_root (1 << 3)
#define conn_op_was_polling   (1 << 4)

typedef cuint_t cherokee_connection_options_t;


struct cherokee_connection {
	cherokee_list_t               list_node;

	/* References
	 */
	void                         *server;
	void                         *vserver;
	void                         *thread;

	/* ID
	 */
	culong_t                      id;
	cherokee_buffer_t             self_trace;
	
	/* Socket stuff
	 */
	cherokee_socket_t             socket;
	cherokee_http_upgrade_t       upgrade;
	cherokee_connection_options_t options;

	cherokee_logger_t            *logger_ref;
	cherokee_handler_t           *handler;

	/* Buffers
	 */
	cherokee_buffer_t             incoming_header;  /* -> header               */
	cherokee_buffer_t             header_buffer;    /* <- header, -> post data */
	cherokee_buffer_t             buffer;           /* <- data                 */

	/* State
	 */
	cherokee_connection_phase_t   phase;
	cherokee_http_t               error_code;
	
	/* Headers
	 */
	cherokee_header_t             header;

	/* Encoders
	 */
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
	cherokee_buffer_t             effective_directory;
	cherokee_buffer_t             redirect;
	cherokee_buffer_t             request_original;

	/* Authentication
	 */
	cherokee_validator_t         *validator;           /* Validator object */
	cherokee_buffer_t            *realm_ref;           /* "My private data" */

	cherokee_http_auth_t          auth_type;           /* Auth type of the resource */
	cherokee_http_auth_t          req_auth_type;       /* Auth type of the request  */

	/* Traffic
	 */
	size_t                        rx;                  /* Bytes received */
	size_t                        rx_partial;          /* RX partial counter */
	size_t                        tx;                  /* Bytes sent */
	size_t                        tx_partial;          /* TX partial counter */
	time_t                        traffic_next;        /* Time to update traffic */

	/* Post info
	 */
	cherokee_post_t               post;

	uint32_t                      keepalive;
	time_t                        timeout;

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

	int                           regex_ovector[OVECTOR_LEN];
	int                           regex_ovecsize;
	
	/* Content Expiration
	 */
	cherokee_expiration_t         expiration;
	time_t                        expiration_time;

	/* Chunked encoding
	 */
	cherokee_boolean_t            chunked_encoding;
	cherokee_boolean_t            chunked_last_package;
	cherokee_buffer_t             chunked_len;
	size_t                        chunked_sent;
	struct iovec                  chunks[3];
	uint16_t                      chunksn;
};

#define CONN_SRV(c)    (SRV(CONN(c)->server))
#define CONN_HDR(c)    (HDR(CONN(c)->header))
#define CONN_SOCK(c)   (SOCKET(CONN(c)->socket))
#define CONN_VSRV(c)   (VSERVER(CONN(c)->vserver))
#define CONN_THREAD(c) (THREAD(CONN(c)->thread))

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
ret_t cherokee_connection_send_switching         (cherokee_connection_t *conn);
ret_t cherokee_connection_recv                   (cherokee_connection_t *conn, cherokee_buffer_t *buffer, off_t *len);

/* Internal
 */
ret_t cherokee_connection_create_handler         (cherokee_connection_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_connection_create_encoder         (cherokee_connection_t *conn, cherokee_avl_t *encoders, cherokee_avl_t *accept_enc);
ret_t cherokee_connection_setup_error_handler    (cherokee_connection_t *conn);
ret_t cherokee_connection_check_authentication   (cherokee_connection_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_connection_check_ip_validation    (cherokee_connection_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_connection_check_only_secure      (cherokee_connection_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_connection_check_http_method      (cherokee_connection_t *conn, cherokee_config_entry_t *config_entry);
void  cherokee_connection_set_keepalive          (cherokee_connection_t *conn);

/* Iteration
 */
ret_t cherokee_connection_open_request           (cherokee_connection_t *conn);
ret_t cherokee_connection_reading_check          (cherokee_connection_t *conn);
ret_t cherokee_connection_step                   (cherokee_connection_t *conn);

/* Headers
 */
ret_t cherokee_connection_build_header           (cherokee_connection_t *conn);
ret_t cherokee_connection_get_request            (cherokee_connection_t *conn);
ret_t cherokee_connection_parse_range            (cherokee_connection_t *conn);
int   cherokee_connection_is_userdir             (cherokee_connection_t *conn);
ret_t cherokee_connection_build_local_directory  (cherokee_connection_t *conn, cherokee_virtual_server_t *vsrv, cherokee_config_entry_t *entry);
ret_t cherokee_connection_build_local_directory_userdir (cherokee_connection_t *conn, cherokee_virtual_server_t *vsrv, cherokee_config_entry_t *entry);
ret_t cherokee_connection_clean_error_headers    (cherokee_connection_t *conn);
ret_t cherokee_connection_set_redirect           (cherokee_connection_t *conn, cherokee_buffer_t *address);

ret_t cherokee_connection_clean_for_respin       (cherokee_connection_t *conn);
int   cherokee_connection_use_webdir             (cherokee_connection_t *conn);

/* Log
 */
ret_t cherokee_connection_log_or_delay           (cherokee_connection_t *conn);
ret_t cherokee_connection_log_delayed            (cherokee_connection_t *conn);
ret_t cherokee_connection_update_vhost_traffic   (cherokee_connection_t *conn);
char *cherokee_connection_print                  (cherokee_connection_t *conn);

/* Transfers
 */
void cherokee_connection_rx_add                  (cherokee_connection_t *conn, ssize_t rx);
void cherokee_connection_tx_add                  (cherokee_connection_t *conn, ssize_t tx);

#endif /* CHEROKEE_CONNECTION_PROTECTED_H */
