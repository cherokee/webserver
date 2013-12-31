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
#include "proxy.h"
#include "util.h"

#define ENTRIES            "proxy"
#define PROXY_ENV_VAR      "http_proxy"
#define PROXY_PORT_DEFAULT 8080


static ret_t
find_proxy_environment (cherokee_buffer_t *proxy, cuint_t *port)
{
	char *env;
	char *colon;

	env = getenv (PROXY_ENV_VAR);
	if (!env) return ret_not_found;

	colon = strrchr(env, ':');
	if (colon) {
		cherokee_buffer_add (proxy, env, colon-env);
		*port = atoi(colon+1);
	} else {
		cherokee_buffer_add (proxy, env, strlen(env));
		*port = PROXY_PORT_DEFAULT;
	}

	return ret_ok;
}


ret_t
cget_find_proxy (cherokee_buffer_t *proxy, cuint_t *port)
{
	ret_t ret;

	ret = find_proxy_environment (proxy, port);
	if (ret == ret_ok) goto found;

	TRACE(ENTRIES, "Not found %s\n", "");
	return ret_not_found;

found:
	TRACE(ENTRIES, "proxy='%s' port=%d\n", proxy->buf, *port);
	return ret_ok;
}
