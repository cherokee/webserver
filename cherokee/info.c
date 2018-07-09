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
#include "info.h"
#include "plugin_loader.h"
#include "server-protected.h"

void
cherokee_info_build_print (cherokee_server_t *srv)
{
	cherokee_buffer_t builtin = CHEROKEE_BUF_INIT;

	/* Basic info
	 */
	printf ("Compilation\n");
	printf (" Version: " PACKAGE_VERSION "\n");
	printf (" Compiled on: " __DATE__ " " __TIME__ "\n");
	printf (" Arguments to configure: " CHEROKEE_CONFIG_ARGS "\n");
	printf ("\n");

	/* Paths
	 */
	printf ("Installation\n");
	printf (" Deps dir: " CHEROKEE_DEPSDIR "\n");
	printf (" Data dir: " CHEROKEE_DATADIR "\n");
	printf (" Icons dir: " CHEROKEE_ICONSDIR "\n");
	printf (" Themes dir: " CHEROKEE_THEMEDIR "\n");
	printf (" Plug-in dir: " CHEROKEE_PLUGINDIR "\n");
	printf (" Temporal dir: " TMPDIR "\n");
	printf ("\n");

	/* Print plug-ins information
	 */
	printf ("Plug-ins\n");
	cherokee_plugin_loader_get_mods_info (&srv->loader, &builtin);
	printf (" Built-in: %s\n", builtin.buf ? builtin.buf : "");
	printf ("\n");
	cherokee_buffer_mrproper (&builtin);

	/* Support
	 */
	printf ("Support\n");
#ifdef HAVE_IPV6
	printf (" IPv6: yes\n");
#else
	printf (" IPv6: no\n");
#endif
#ifdef HAVE_PTHREAD
	printf (" Pthreads: yes\n");
#else
	printf (" Pthreads: no\n");
#endif
#ifdef TRACE_ENABLED
	printf (" Tracing: yes\n");
#else
	printf (" Tracing: no\n");
#endif
#ifdef WITH_SENDFILE
	printf (" sendfile(): yes\n");
#else
	printf (" sendfile(): no\n");
#endif
#ifdef HAVE_SYSLOG
	printf (" syslog(): yes\n");
#else
	printf (" syslog(): no\n");
#endif

	printf (" Polling methods: ");
#ifdef HAVE_EPOLL
	printf ("epoll ");
#endif
#ifdef HAVE_KQUEUE
	printf ("kqueue ");
#endif
#ifdef HAVE_POLL
	printf ("poll ");
#endif
#ifdef HAVE_PORT
	printf ("ports ");
#endif
#ifdef HAVE_SELECT
	printf ("select ");
#endif
	printf ("\n");

#ifdef HAVE_OPENSSL
	printf (" SSL/TLS: libssl\n");
# ifndef OPENSSL_NO_TLSEXT
	printf (" TLS SNI: yes\n");
# else
	printf (" TLS SNI: no\n");
# endif
#else
	printf (" SSL/TLS: no\n");
	printf (" TLS SNI: no\n");
#endif

	printf ("\n");
}
