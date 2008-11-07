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

#include "common-internal.h"
#include "proxy_hosts.h"


static void
poll_free (void *p)
{
	cherokee_handler_proxy_poll_t *poll = p;
	poll = p;
}

ret_t
cherokee_handler_proxy_hosts_init (cherokee_handler_proxy_hosts_t *hosts)
{
 	CHEROKEE_MUTEX_INIT (&hosts->hosts_mutex, NULL);
	cherokee_avl_init (&hosts->hosts);
	cherokee_buffer_init (&hosts->tmp);

	return ret_ok;
}

ret_t
cherokee_handler_proxy_hosts_mrproper (cherokee_handler_proxy_hosts_t *hosts)
{
	CHEROKEE_MUTEX_DESTROY (&hosts->hosts_mutex);
	cherokee_avl_mrproper (&hosts->hosts, poll_free);
	cherokee_buffer_mrproper (&hosts->tmp);

	return ret_ok;
}

ret_t
cherokee_handler_proxy_hosts_get (cherokee_handler_proxy_hosts_t  *hosts,
				  cherokee_source_t               *src,
				  cherokee_handler_proxy_poll_t  **poll)
{
	ret_t ret;

	CHEROKEE_MUTEX_LOCK (&hosts->hosts_mutex);

	/* Build the index name */
	cherokee_buffer_clean       (&hosts->tmp);
	cherokee_buffer_add_buffer  (&hosts->tmp, &src->host);
	cherokee_buffer_add_char    (&hosts->tmp, ':');
	cherokee_buffer_add_ulong10 (&hosts->tmp, src->port);

	/* Check the hosts tree */
	ret = cherokee_avl_get (&hosts->hosts, &hosts->tmp, (void **)poll);
	switch (ret) {
	case ret_ok:
		break;
	case ret_not_found: {
		CHEROKEE_NEW (n, handler_proxy_poll);
		cherokee_avl_add (&hosts->hosts, &hosts->tmp, n);
		*poll = n;
		break;
	}
	default:
		goto error;
	}

	/* Got it */
	CHEROKEE_MUTEX_UNLOCK (&hosts->hosts_mutex);
	return ret_ok;

error:
	CHEROKEE_MUTEX_LOCK (&hosts->hosts_mutex);
	return ret_error;
}


/* Polls
 */

ret_t
cherokee_handler_proxy_poll_new (cherokee_handler_proxy_poll_t **poll)
{
	CHEROKEE_NEW_STRUCT (n, handler_proxy_poll);

	INIT_LIST_HEAD (&n->active);
	INIT_LIST_HEAD (&n->reuse);
	CHEROKEE_MUTEX_INIT (&n->mutex, NULL);

	*poll = n;
	return ret_ok;
}


ret_t
cherokee_handler_proxy_poll_free (cherokee_handler_proxy_poll_t *poll)
{
	cherokee_list_t *i, *j;

	list_for_each_safe (i, j, &poll->active) {
	}

	list_for_each_safe (i, j, &poll->reuse) {
	}

	CHEROKEE_MUTEX_DESTROY (&poll->mutex);
	return ret_ok;
}


static ret_t
init_socket (cherokee_socket_t *socket, 
	     cherokee_source_t *src)
{
	ret_t ret;

	/* Family */
	ret = cherokee_socket_set_client (socket, AF_INET);
        if (unlikely(ret != ret_ok)) 
		return ret_error;

	/* TCP port */
        SOCKET_SIN_PORT(socket) = htons (src->port);

        /* IP host */
        ret = cherokee_socket_pton (socket, &src->host);
        if (ret != ret_ok) {
                ret = cherokee_socket_gethostbyname (socket, &src->host);
                if (unlikely(ret != ret_ok))
			return ret_error;
        }

	return ret_ok;
}


ret_t
cherokee_handler_proxy_poll_get (cherokee_handler_proxy_poll_t  *poll,
				 cherokee_handler_proxy_conn_t **pconn,
				 cherokee_source_t              *src)
{
	ret_t            ret;
	cherokee_list_t *i;

	CHEROKEE_MUTEX_LOCK (&poll->mutex);

	if (! cherokee_list_empty (&poll->reuse)) {
		/* Reuse a prev connection */
		i = poll->reuse.next;
		cherokee_list_del (i);
		cherokee_list_add (i, &poll->active);

		*pconn = PROXY_CONN(i);

	} else {
		cherokee_handler_proxy_conn_t *n;

		/* Create a new connection */
		ret = cherokee_handler_proxy_conn_new (&n);
		if (ret != ret_ok)
			goto error;

		ret = init_socket (&n->socket, src);
		if (ret != ret_ok)
			goto error;

		cherokee_list_add (&n->listed, &poll->active);
		n->poll_ref = poll;

		*pconn = n;
	}	

	CHEROKEE_MUTEX_UNLOCK (&poll->mutex);
	return ret_ok;
error:
	CHEROKEE_MUTEX_UNLOCK (&poll->mutex);
	return ret_ok;
}

static ret_t
poll_release (cherokee_handler_proxy_poll_t *poll,
	      cherokee_handler_proxy_conn_t *pconn)
{
	cherokee_list_add (&pconn->listed, &poll->reuse);	
	return ret_ok;
}
	

/* Conn's
 */

ret_t
cherokee_handler_proxy_conn_new (cherokee_handler_proxy_conn_t **pconn)
{
	CHEROKEE_NEW_STRUCT (n, handler_proxy_conn);
	cherokee_socket_init (&n->socket);

	cherokee_buffer_init (&n->header_in_raw);
	cherokee_buffer_ensure_size (&n->header_in_raw, 512);

	n->poll_ref      = NULL;
	n->keepalive_in  = false;
	n->size_in       = 0;
	n->sent_out      = 0;
	n->enc           = pconn_enc_none;

	*pconn = n;
	return ret_ok;
}


ret_t
cherokee_handler_proxy_conn_free (cherokee_handler_proxy_conn_t *pconn)
{
	cherokee_socket_mrproper (&pconn->socket);
	cherokee_buffer_mrproper (&pconn->header_in_raw);

	return ret_ok;
}


ret_t
cherokee_handler_proxy_conn_release (cherokee_handler_proxy_conn_t *pconn)
{
	return poll_release (pconn->poll_ref, pconn);
}


ret_t
cherokee_handler_proxy_conn_send (cherokee_handler_proxy_conn_t *pconn,
				  cherokee_buffer_t             *buf)
{
	ret_t  ret;
	size_t sent = 0;

	ret = cherokee_socket_bufwrite (&pconn->socket, buf, &sent);
	if (unlikely(ret != ret_ok)) 
		return ret;

	if (sent > buf->len)
		cherokee_buffer_clean (buf);

	cherokee_buffer_move_to_begin (buf, sent);
	return ret_ok;
}

ret_t
cherokee_handler_proxy_conn_recv_headers (cherokee_handler_proxy_conn_t *pconn,
					  cherokee_buffer_t             *body)
{
	ret_t   ret;
	char   *end;
	size_t  size = 0;

	/* Read */
	ret = cherokee_socket_bufread (&pconn->socket,
				       &pconn->header_in_raw,
				       512, &size);
	switch (ret) {
	case ret_ok:
		break;
	case ret_eof:
	case ret_error:
	case ret_eagain:
		return ret;
	default:
		RET_UNKNOWN(ret);
	}

	/* Look for the end of header */
	end = strnstr (pconn->header_in_raw.buf, CRLF_CRLF, pconn->header_in_raw.len);
	if (end == NULL) {
		return ret_eagain;
	}

	/* Copy the body if there is any */
	size = (pconn->header_in_raw.buf + pconn->header_in_raw.len) - (end + 4);

	cherokee_buffer_add (body, end+4, size);
	cherokee_buffer_drop_ending (&pconn->header_in_raw, size);

	return ret_ok;
}

