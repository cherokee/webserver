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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <cherokee/cherokee.h>

#define ERROR       1
#define WATCH_SLEEP 1000

#define CHECK_ERROR(msg)             \
	if (ret != ret_ok) {        \
		PRINT_ERROR_S (msg"\n"); \
		return ERROR;            \
	}

int
main (int argc, char *argv[])
{
	ret_t                    ret;
	cuint_t                  fds_num;
	cherokee_fdpoll_t       *fdpoll;
	cherokee_admin_client_t *client;
	cherokee_list_t         *i, *tmp;

	cuint_t                  port;
	cherokee_buffer_t        buf;
	cherokee_list_t          conns = LIST_HEAD_INIT(conns);

	if (argc <= 1) {
		PRINT_ERROR ("%s url request\n", argv[0]);
		return 1;
	}

	/* Build everyhing
	 */
	cherokee_sys_fdlimit_get (&fds_num);

	ret = cherokee_fdpoll_best_new (&fdpoll, fds_num, fds_num);
	if (ret != ret_ok) return ERROR;

	ret = cherokee_admin_client_new (&client);
	if (ret != ret_ok) return ERROR;

	/* Set the request
	 */
	cherokee_buffer_init (&buf);
	cherokee_buffer_add (&buf, argv[1], strlen(argv[1]));

	/* Prepare the admin client object
	 */
	ret = cherokee_admin_client_prepare (client, fdpoll, &buf);
	if (ret != ret_ok) {
		PRINT_ERROR_S ("Client prepare failed\n");

		cherokee_buffer_mrproper (&buf);
		return ERROR;
	}

	ret = cherokee_admin_client_connect (client);
	if (ret != ret_ok) {
		PRINT_ERROR_S ("Couldn't connect\n");

		cherokee_buffer_mrproper (&buf);
		return ERROR;
	}

	/* Process it
	 */
	cherokee_buffer_clean (&buf);

	RUN_CLIENT1 (client, fdpoll, cherokee_admin_client_ask_port, &port);
	CHECK_ERROR ("port");
	printf ("Port is %d\n", port);

	RUN_CLIENT1 (client, cherokee_admin_client_ask_port_tls, &port);
	CHECK_ERROR ("port_tls");
	printf ("Port TLS is %d\n", port);

	RUN_CLIENT1 (client, cherokee_admin_client_ask_rx, &buf);
	CHECK_ERROR ("rx");
	printf ("Server RX is %s\n", buf.buf);
	cherokee_buffer_clean (&buf);

	RUN_CLIENT1 (client, cherokee_admin_client_ask_tx, &buf);
	CHECK_ERROR ("tx");
	printf ("Server TX is %s\n", buf.buf);

	RUN_CLIENT1 (client, fdpoll, cherokee_admin_client_ask_connections, &conns);
	CHECK_ERROR ("conns");

	{
		list_for_each (i, &conns) {
			cherokee_connection_info_t *conn = CONN_INFO(i);

			printf ("Request: '%s', phase: '%s', rx: '%s', tx: '%s', size: '%s'\n",
			        conn->request.buf, conn->phase.buf, conn->rx.buf,
			        conn->tx.buf, conn->total_size.buf);
		}
	}

	list_for_each_safe (i, tmp, &conns) {
		cherokee_connection_info_free (CONN_INFO(i));
	}

	cherokee_buffer_mrproper (&buf);

	/* Clean up
	 */
	cherokee_admin_client_free (client);
	cherokee_fdpoll_free (fdpoll);

	return 0;
}
