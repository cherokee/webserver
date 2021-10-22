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
#ifdef HAVE_OPENSSL
#  include "cryptor_libssl.h"
#endif

void
cherokee_info_build_print (cherokee_server_t *srv)
{
	cherokee_buffer_t tmp       = CHEROKEE_BUF_INIT;
	cherokee_buffer_t prot_buf  = CHEROKEE_BUF_INIT;
	cherokee_buffer_t deact_buf = CHEROKEE_BUF_INIT;
	ret_t ret;

	/* Basic info
	 */
	printf ("Compilation\n");
	printf (" Version: " PACKAGE_VERSION "\n");
	printf (" Compiled on: " __DATE__ " " __TIME__ "\n");
	printf (" Arguments to configure: " CHEROKEE_CONFIG_ARGS "\n");
#ifdef HAVE_OPENSSL
	printf (" OpenSSL support: libssl (" OPENSSL_VERSION_TEXT ")\n");
#endif
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
	cherokee_plugin_loader_get_mods_info (&srv->loader, &tmp);
	printf (" Built-in: %s\n", tmp.buf ? tmp.buf : "");
	printf ("\n");
        cherokee_buffer_clean(&tmp);

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
        cherokee_buffer_clean (&prot_buf);
        cherokee_buffer_clean (&deact_buf);
	if (srv->cryptor != NULL && srv->cryptor->tls_info != NULL) {
                ret = srv->cryptor->tls_info (srv->cryptor,
                                              &tmp,
                                              &prot_buf,
                                              &deact_buf);
                if (ret == ret_ok) {
                        printf (" SSL/TLS: libssl (%s)\n", tmp.buf);
                        if (! cherokee_buffer_is_empty(&prot_buf)) {
                                printf ("          supported protocols: %s\n",
                                        prot_buf.buf);
                        }
                        if (! cherokee_buffer_is_empty(&deact_buf)) {
                                printf ("          protocols deactivated by maintainer: %s\n",
                                        deact_buf.buf);
                        }
                }
        }
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

	cherokee_buffer_mrproper (&tmp);
	cherokee_buffer_mrproper (&prot_buf);
	cherokee_buffer_mrproper (&deact_buf);
}
