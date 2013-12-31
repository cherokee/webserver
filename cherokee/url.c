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

#include "common-internal.h"
#include "url.h"

#include <strings.h>

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif


ret_t
cherokee_url_init (cherokee_url_t *url)
{
	ret_t ret;

	/* New buffer objects
	 */
	ret = cherokee_buffer_init (&url->host);
	if (unlikely(ret < ret_ok)) return ret;

	ret = cherokee_buffer_init (&url->request);
	if (unlikely(ret < ret_ok)) return ret;

	/* Set default values
	 */
	url->port = 80;
	return ret_ok;
}


ret_t
cherokee_url_mrproper (cherokee_url_t *url)
{
	cherokee_buffer_mrproper (&url->host);
	cherokee_buffer_mrproper (&url->request);
	return ret_ok;
}


ret_t
cherokee_url_clean (cherokee_url_t *url)
{
	url->port = 80;

	cherokee_buffer_clean (&url->host);
	cherokee_buffer_clean (&url->request);

	return ret_ok;
}


static ret_t
parse_protocol (cherokee_url_t *url, char *string, cuint_t *len)
{
	/* Drop "http://"
	 */
	if (strncasecmp("http://", string, 7) == 0) {
		url->protocol = http;
		*len          = 7;

		url->port = 80;
		return ret_ok;
	}

	/* Drop "https://"
	 */
	if (strncasecmp("https://", string, 8) == 0) {
		url->protocol = https;
		*len          = 8;

		url->port = 443;
		return ret_ok;
	}

	return ret_not_found;
}


static ret_t
cherokee_url_parse_guts (cherokee_url_t    *url,
                         cherokee_buffer_t *url_buf,
                         cherokee_buffer_t *user_ret,
                         cherokee_buffer_t *password_ret)
{
	ret_t    ret;
	cuint_t  len = 0 ;
	char    *port;
	char    *slash;
	char    *server;
	char    *arroba;
	char    *tmp;

	/* Drop protocol, if exists..
	 */
	ret = parse_protocol (url, url_buf->buf, &len);
	if (unlikely(ret < ret_ok)) return ret_error;

	tmp = url_buf->buf + len;

	/* User (and password)
	 */
	arroba = strchr (tmp, '@');
	if (arroba != NULL) {
		char *sep;

		sep = strchr (tmp, ':');
		if (sep == NULL) {
			cherokee_buffer_clean (user_ret);
			cherokee_buffer_add (user_ret, tmp, arroba - tmp);
		} else {
			cherokee_buffer_clean (user_ret);
			cherokee_buffer_add (user_ret, tmp, sep - tmp);
			sep++;
			cherokee_buffer_clean (password_ret);
			cherokee_buffer_add (password_ret, sep, arroba - sep);
		}

		tmp = arroba + 1;
	}

	/* Split the host/request
	 */
	server = tmp;
	len    = strlen (server);
	slash  = strpbrk (server, "/\\");

	if (slash == NULL) {
		cherokee_buffer_add (&url->request, "/", 1);
		cherokee_buffer_add (&url->host, server, len);
	} else {
		cherokee_buffer_add (&url->request, slash, len-(slash-server));
		cherokee_buffer_add (&url->host, server, slash-server);
	}

	/* Drop up the port, if exists..
	 */
	port = strchr (url->host.buf, ':');
	if (port != NULL) {

		/* Read port number
		 */
		if (slash != NULL) *slash = '\0';
		URL_PORT(url) = atoi (port+1);
		if (slash != NULL) *slash =  '/';

		/* .. and remove it
		 */
		ret = cherokee_buffer_drop_ending (&url->host, strlen(port));
		if (unlikely(ret < ret_ok)) return ret;
	}

#if 0
	cherokee_url_print (url);
#endif

	return ret_ok;
}


ret_t
cherokee_url_parse (cherokee_url_t    *url,
                    cherokee_buffer_t *string,
                    cherokee_buffer_t *user_ret,
                    cherokee_buffer_t *password_ret)
{
	if (cherokee_buffer_is_empty (string)) {
		return ret_error;
	}

	return cherokee_url_parse_guts (url, string, user_ret, password_ret);
}


ret_t
cherokee_url_build_string (cherokee_url_t *url, cherokee_buffer_t *buf)
{
	cherokee_buffer_add_buffer (buf, &url->host);

	if (! http_port_is_standard (url->port, (url->protocol == https)))
	{
		cherokee_buffer_add_char (buf, ':');
		cherokee_buffer_add_ulong10 (buf, (culong_t) url->port);
	}

	cherokee_buffer_add_buffer (buf, &url->request);

	return ret_ok;
}


ret_t
cherokee_url_print (cherokee_url_t *url)
{
	printf ("Host:    %s\n", url->host.buf);
	printf ("Request: %s\n", url->request.buf);
	printf ("Port:    %d\n", url->port);
	return ret_ok;
}
