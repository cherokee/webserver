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
#include "handler_render_rrd.h"
#include "server-protected.h"
#include "connection-protected.h"
#include "thread.h"
#include "rrd_tools.h"
#include "util.h"

#define ENTRIES "rrd,render,render_rrd,handler"

#define DISABLED_MSG "Graphs generation is disabled because RRDtool was not found." CRLF

PLUGIN_INFO_HANDLER_EASY_INIT (render_rrd, http_get | http_head);


/* Functions
 */
static ret_t
find_interval (const char                         *txt,
               cherokee_collector_rrd_interval_t **interval)
{
	cherokee_collector_rrd_interval_t *i;

	for (i = cherokee_rrd_intervals; i->interval != NULL; i++) {
		if (strncmp (i->interval, txt, 2) == 0) {
			*interval = i;
			return ret_ok;
		}
	}

	return ret_error;
}


static cherokee_boolean_t
check_image_freshness (cherokee_buffer_t                 *buf,
                       cherokee_collector_rrd_interval_t *interval)
{
	int         re;
	struct stat info;

	/* cache_img_dir + "/" + buf + "_" + interval + ".png"
	 */
	cherokee_buffer_prepend_str (buf, "/");
	cherokee_buffer_prepend_buf (buf, &rrd_connection->path_img_cache);

	cherokee_buffer_add_char    (buf, '_');
	cherokee_buffer_add         (buf, interval->interval, strlen(interval->interval));
	cherokee_buffer_add_str     (buf, ".png");

	re = cherokee_stat (buf->buf, &info);
	if (re != 0) {
		TRACE(ENTRIES, "Image does not exist: '%s'\n", buf->buf);
		return false;
	}

	if (cherokee_bogonow_now >= info.st_mtime + interval->secs_per_pixel) {
		TRACE(ENTRIES, "Image is too old: '%s'. It was valid for %d secs, but it's %d secs old.\n",
		      buf->buf, interval->secs_per_pixel, (cherokee_bogonow_now - info.st_mtime));
		return false;
	}

	TRACE(ENTRIES, "Image is fresh enough: '%s'\n", buf->buf);
	return true;
}


static ret_t
command_rrdtool (cherokee_handler_render_rrd_t *hdl,
                 cherokee_buffer_t             *buf)
{
	ret_t ret;

	/* Execute
	 */
	ret = cherokee_rrd_connection_execute (rrd_connection, buf);
	if (ret != ret_ok) {
		LOG_ERROR (CHEROKEE_ERROR_HANDLER_RENDER_RRD_EXEC, buf->buf);
		cherokee_rrd_connection_kill (rrd_connection, false);
		return ret_error;
	}

	/* Check everything was alright
	 */
	if (cherokee_buffer_is_empty (buf)) {
		LOG_ERROR_S (CHEROKEE_ERROR_HANDLER_RENDER_RRD_EMPTY_REPLY);
		return ret_error;

	} else if (strncmp (buf->buf, "ERROR", 5) == 0) {
		cherokee_buffer_add_buffer (&hdl->rrd_error, buf);
		LOG_ERROR (CHEROKEE_ERROR_HANDLER_RENDER_RRD_MSG, buf->buf);

		return ret_error;
	}

	TRACE(ENTRIES, "Command execute. Everything is fine.\n");
	return ret_ok;
}


static ret_t
render_srv_accepts (cherokee_handler_render_rrd_t     *hdl,
                    cherokee_collector_rrd_interval_t *interval)
{
	cherokee_buffer_t *tmp = &rrd_connection->tmp;

	cherokee_buffer_add_str    (tmp, "graph ");
	cherokee_buffer_add_buffer (tmp, &rrd_connection->path_img_cache);
	cherokee_buffer_add_va     (tmp, "/server_accepts_%s.png ", interval->interval);
	cherokee_buffer_add_va     (tmp, "--imgformat PNG --width 580 --height 340 --start -%s ", interval->interval);
	cherokee_buffer_add_va     (tmp, "--title \"Accepted Connections: %s\" ", interval->interval);
	cherokee_buffer_add_str    (tmp, "--vertical-label \"conn/s\" -c BACK#FFFFFF -c SHADEA#FFFFFF -c SHADEB#FFFFFF -c SHADEA#FFFFFF -c SHADEB#FFFFFF ");

	cherokee_buffer_add_va     (tmp, "DEF:accepts=%s/server.rrd:Accepts:AVERAGE ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_va     (tmp, "DEF:accepts_min=%s/server.rrd:Accepts:MIN ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_va     (tmp, "DEF:accepts_max=%s/server.rrd:Accepts:MAX ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_str    (tmp, "VDEF:accepts_total=accepts,TOTAL ");
	cherokee_buffer_add_str    (tmp, "CDEF:accepts_minmax=accepts_max,accepts_min,- ");

	cherokee_buffer_add_va     (tmp, "DEF:requests=%s/server.rrd:Requests:AVERAGE ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_va     (tmp, "DEF:requests_min=%s/server.rrd:Requests:MIN ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_va     (tmp, "DEF:requests_max=%s/server.rrd:Requests:MAX ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_str    (tmp, "VDEF:requests_total=requests,TOTAL ");
	cherokee_buffer_add_str    (tmp, "CDEF:requests_minmax=requests_max,requests_min,- ");

	cherokee_buffer_add_str    (tmp, "LINE1.5:requests#900:\"HTTP reqs\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:requests:LAST:\"Current\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:requests:AVERAGE:\" Average\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:requests_max:MAX:\" Maximum\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:requests_total:\"  Total\\:%8.2lf%s\\n\" ");

	cherokee_buffer_add_str    (tmp, "AREA:accepts_min#ffffff: ");
	cherokee_buffer_add_str    (tmp, "STACK:accepts_minmax#4477BB: ");
	cherokee_buffer_add_str    (tmp, "LINE1.5:accepts#224499:\"TCP conns\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:accepts:LAST:\"Current\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:accepts:AVERAGE:\" Average\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:accepts_max:MAX:\" Maximum\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:accepts_total:\"  Total\\:%8.2lf%s\\n\" ");
	cherokee_buffer_add_str    (tmp, "\n");

	command_rrdtool (hdl, tmp);
	cherokee_buffer_clean (tmp);

	return ret_ok;
}


static ret_t
render_srv_timeouts (cherokee_handler_render_rrd_t     *hdl,
                     cherokee_collector_rrd_interval_t *interval)
{
	cherokee_buffer_t *tmp = &rrd_connection->tmp;

	cherokee_buffer_add_str    (tmp, "graph ");
	cherokee_buffer_add_buffer (tmp, &rrd_connection->path_img_cache);
	cherokee_buffer_add_va     (tmp, "/server_timeouts_%s.png ", interval->interval);
	cherokee_buffer_add_va     (tmp, "--imgformat PNG --width 580 --height 340 --start -%s ", interval->interval);
	cherokee_buffer_add_va     (tmp, "--title \"Timeouts: %s\" ", interval->interval);
	cherokee_buffer_add_str    (tmp, "--vertical-label \"timeouts/s\" -c BACK#FFFFFF -c SHADEA#FFFFFF -c SHADEB#FFFFFF ");
	cherokee_buffer_add_va     (tmp, "DEF:timeouts=%s/server.rrd:Timeouts:AVERAGE ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_va     (tmp, "DEF:timeouts_min=%s/server.rrd:Timeouts:MIN ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_va     (tmp, "DEF:timeouts_max=%s/server.rrd:Timeouts:MAX ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_str    (tmp, "VDEF:timeouts_total=timeouts,TOTAL ");
	cherokee_buffer_add_str    (tmp, "CDEF:timeouts_minmax=timeouts_max,timeouts_min,- ");
	cherokee_buffer_add_str    (tmp, "COMMENT:\"\\n\" ");
	cherokee_buffer_add_str    (tmp, "COMMENT:\"  Current           Average           Maximum             Total\\n\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:timeouts:LAST:\"%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:timeouts:AVERAGE:\"%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:timeouts_max:MAX:\"%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:timeouts_total:\"%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "AREA:timeouts_min#ffffff: ");
	cherokee_buffer_add_str    (tmp, "STACK:timeouts_minmax#C007:Timeouts ");
	cherokee_buffer_add_str    (tmp, "LINE1.5:timeouts#900:Average ");
	cherokee_buffer_add_str    (tmp, "\n");

	command_rrdtool (hdl, tmp);
	cherokee_buffer_clean (tmp);

	return ret_ok;
}


static ret_t
render_srv_traffic (cherokee_handler_render_rrd_t     *hdl,
                    cherokee_collector_rrd_interval_t *interval)
{
	cherokee_buffer_t *tmp = &rrd_connection->tmp;

	cherokee_buffer_add_str    (tmp, "graph ");
	cherokee_buffer_add_buffer (tmp, &rrd_connection->path_img_cache);
	cherokee_buffer_add_va     (tmp, "/server_traffic_%s.png ", interval->interval);
	cherokee_buffer_add_va     (tmp, "--imgformat PNG --width 580 --height 340 --start -%s ", interval->interval);
	cherokee_buffer_add_va     (tmp, "--title \"Traffic: %s\" ", interval->interval);
	cherokee_buffer_add_str    (tmp, "--vertical-label \"bytes/s\" -c BACK#FFFFFF -c SHADEA#FFFFFF -c SHADEB#FFFFFF ");
	cherokee_buffer_add_va     (tmp, "DEF:rx=%s/server.rrd:RX:AVERAGE ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_va     (tmp, "DEF:rx_max=%s/server.rrd:RX:MAX ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_va     (tmp, "DEF:tx=%s/server.rrd:TX:AVERAGE ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_va     (tmp, "DEF:tx_max=%s/server.rrd:TX:MAX ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_str    (tmp, "VDEF:tx_total=tx,TOTAL ");
	cherokee_buffer_add_str    (tmp, "VDEF:rx_total=rx,TOTAL ");
	cherokee_buffer_add_str    (tmp, "CDEF:rx_r=rx,-1,* ");
	cherokee_buffer_add_str    (tmp, "AREA:tx#4477BB:Out ");
	cherokee_buffer_add_str    (tmp, "LINE1:tx#224499 ");
	cherokee_buffer_add_str    (tmp, "GPRINT:tx:LAST:\"     Current\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:tx:AVERAGE:\" Average\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:tx_max:MAX:\" Maximum\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:tx_total:\"   Total\\:%8.2lf%s\\n\" ");
	cherokee_buffer_add_str    (tmp, "AREA:rx_r#C007 ");
	cherokee_buffer_add_str    (tmp, "LINE1:rx_r#990000:In ");
	cherokee_buffer_add_str    (tmp, "GPRINT:rx:LAST:\"      Current\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:rx:AVERAGE:\" Average\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:rx_max:MAX:\" Maximum\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:rx_total:\"   Total\\:%8.2lf%s\\n\" ");
	cherokee_buffer_add_str    (tmp, "\n");

	command_rrdtool (hdl, tmp);
	cherokee_buffer_clean (tmp);

	return ret_ok;
}


static ret_t
render_vsrv_traffic (cherokee_handler_render_rrd_t     *hdl,
                     cherokee_collector_rrd_interval_t *interval,
                     cherokee_buffer_t                 *vserver_name)
{
	cherokee_buffer_t *tmp = &rrd_connection->tmp;

	cherokee_buffer_add_str    (tmp, "graph ");
	cherokee_buffer_add_buffer (tmp, &rrd_connection->path_img_cache);
	cherokee_buffer_add_va     (tmp, "'/vserver_traffic_%s_%s.png' ", vserver_name->buf, interval->interval);
	cherokee_buffer_add_va     (tmp, "--imgformat PNG --width 580 --height 340 --start -%s ", interval->interval);
	cherokee_buffer_add_va     (tmp, "--title \"Traffic, %s: %s\" ", vserver_name->buf, interval->interval);
	cherokee_buffer_add_str    (tmp, "--vertical-label \"bytes/s\" -c BACK#FFFFFF -c SHADEA#FFFFFF -c SHADEB#FFFFFF ");
	cherokee_buffer_add_va     (tmp, "'DEF:rx=%s/vserver_%s.rrd:RX:AVERAGE' ", rrd_connection->path_databases.buf, vserver_name->buf);
	cherokee_buffer_add_va     (tmp, "'DEF:rx_max=%s/vserver_%s.rrd:RX:MAX' ", rrd_connection->path_databases.buf, vserver_name->buf);
	cherokee_buffer_add_va     (tmp, "'DEF:tx=%s/vserver_%s.rrd:TX:AVERAGE' ", rrd_connection->path_databases.buf, vserver_name->buf);
	cherokee_buffer_add_va     (tmp, "'DEF:tx_max=%s/vserver_%s.rrd:TX:MAX' ", rrd_connection->path_databases.buf, vserver_name->buf);
	cherokee_buffer_add_str    (tmp, "VDEF:tx_total=tx,TOTAL ");
	cherokee_buffer_add_str    (tmp, "VDEF:rx_total=rx,TOTAL ");
	cherokee_buffer_add_str    (tmp, "CDEF:rx_r=rx,-1,* ");
	cherokee_buffer_add_str    (tmp, "AREA:tx#4477BB:Out ");
	cherokee_buffer_add_str    (tmp, "LINE1:tx#224499 ");
	cherokee_buffer_add_str    (tmp, "GPRINT:tx:LAST:\"    Current\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:tx:AVERAGE:\" Average\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:tx_max:MAX:\" Maximum\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:tx_total:\"   Total\\:%8.2lf%s\\n\" ");
	cherokee_buffer_add_str    (tmp, "AREA:rx_r#C007 ");
	cherokee_buffer_add_str    (tmp, "LINE1:rx_r#990000:In ");
	cherokee_buffer_add_str    (tmp, "GPRINT:rx:LAST:\"     Current\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:rx:AVERAGE:\" Average\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:rx_max:MAX:\" Maximum\\:%8.2lf%s\" ");
	cherokee_buffer_add_str    (tmp, "GPRINT:rx_total:\"   Total\\:%8.2lf%s\\n\" ");

	/* Total server traffic */
	cherokee_buffer_add_va  (tmp, "DEF:srv_tx=%s/server.rrd:TX:AVERAGE ", rrd_connection->path_databases.buf);
	cherokee_buffer_add_str (tmp, "VDEF:srv_tx_total=srv_tx,TOTAL ");
	cherokee_buffer_add_str (tmp, "LINE1:srv_tx#ADE:\"Global\" ");
	cherokee_buffer_add_str (tmp, "GPRINT:srv_tx:LAST:\" Current\\:%8.2lf%s\" ");
	cherokee_buffer_add_str (tmp, "GPRINT:srv_tx:AVERAGE:\" Average\\:%8.2lf%s\" ");
	cherokee_buffer_add_str (tmp, "GPRINT:srv_tx:MAX:\" Maximum\\:%8.2lf%s\" ");
	cherokee_buffer_add_str (tmp, "GPRINT:srv_tx_total:\"   Total\\:%8.2lf%s\\n\" ");
	cherokee_buffer_add_str (tmp, "\n");

	command_rrdtool (hdl, tmp);
	cherokee_buffer_clean (tmp);

	return ret_ok;
}


ret_t
cherokee_handler_render_rrd_init (cherokee_handler_render_rrd_t *hdl)
{
	ret_t                              ret;
	int                                unlocked;
	const char                        *begin;
	const char                        *dash;
	cherokee_collector_rrd_interval_t *interval;
	cherokee_boolean_t                 fresh;
	cherokee_connection_t             *conn       = HANDLER_CONN(hdl);
	cherokee_buffer_t                  tmp        = CHEROKEE_BUF_INIT;

	/* The handler might be disabled
	 */
	if (HANDLER_RENDER_RRD_PROPS(hdl)->disabled) {
		return ret_ok;
	}

	/* Sanity checks
	 */
	if (strncmp (conn->request.buf + conn->request.len - 4, ".png", 4) != 0) {
		TRACE (ENTRIES, "Malformed RRD image request\n");
		conn->error_code = http_service_unavailable;
		return ret_error;
	}

	/* Parse the interval
	 */
	dash = conn->request.buf + conn->request.len - (3 + 4);

	ret = find_interval (dash+1, &interval);
	if (ret != ret_ok) {
		TRACE (ENTRIES, "No interval detected: %s\n", dash+1);
		conn->error_code = http_service_unavailable;
		return ret_error;
	}

	begin = strrchr (conn->request.buf, '/');
	if (begin == NULL) {
		TRACE (ENTRIES, "Malformed RRD image request. No slash.\n");
		conn->error_code = http_service_unavailable;
		return ret_error;
	}

	/* Launch the task
	 */
	if (! strncmp (begin, "/server_accepts_", 16)) {
		if (strlen(begin) != 18 + 4) {
			TRACE (ENTRIES, "Bad length: %d. Expected 18+4\n", conn->request.len);
			conn->error_code = http_service_unavailable;
			return ret_error;
		}

		cherokee_buffer_add_str (&tmp, "server_accepts");
		fresh = check_image_freshness (&tmp, interval);
		cherokee_buffer_mrproper (&tmp);

		if (! fresh) {
			unlocked = CHEROKEE_MUTEX_TRY_LOCK (&rrd_connection->mutex);
			if (unlocked) {
				cherokee_connection_sleep (conn, 200);
				return ret_eagain;
			}

			ret = render_srv_accepts (hdl, interval);
			if (ret != ret_ok) {
				TRACE (ENTRIES, "Couldn't render image: %s\n", conn->request.buf);

				CHEROKEE_MUTEX_UNLOCK (&rrd_connection->mutex);
				conn->error_code = http_service_unavailable;
				return ret_error;
			}

			CHEROKEE_MUTEX_UNLOCK (&rrd_connection->mutex);
		}

	} else if (! strncmp (begin, "/server_timeouts_", 17)) {
		if (strlen(begin) != 19 + 4) {
			TRACE (ENTRIES, "Bad length: %d. Expected 19+4.\n", conn->request.len);
			conn->error_code = http_service_unavailable;
			return ret_error;
		}

		cherokee_buffer_add_str (&tmp, "server_timeouts");
		fresh = check_image_freshness (&tmp, interval);
		cherokee_buffer_mrproper (&tmp);

		if (! fresh) {
			unlocked = CHEROKEE_MUTEX_TRY_LOCK (&rrd_connection->mutex);
			if (unlocked) {
				cherokee_connection_sleep (conn, 200);
				return ret_eagain;
			}

			ret = render_srv_timeouts (hdl, interval);
			if (ret != ret_ok) {
				TRACE (ENTRIES, "Couldn't render image: %s\n", conn->request.buf);

				CHEROKEE_MUTEX_UNLOCK (&rrd_connection->mutex);
				conn->error_code = http_service_unavailable;
				return ret_error;
			}

			CHEROKEE_MUTEX_UNLOCK (&rrd_connection->mutex);
		}

	} else if (! strncmp (begin, "/server_traffic_", 16)) {
		if (strlen(begin) != 18 + 4) {
			TRACE (ENTRIES, "Bad length: %d. Expected 18+4.\n", conn->request.len);
			conn->error_code = http_service_unavailable;
			return ret_error;
		}

		cherokee_buffer_add_str (&tmp, "server_traffic");
		fresh = check_image_freshness (&tmp, interval);
		cherokee_buffer_mrproper (&tmp);

		if (! fresh) {
			unlocked = CHEROKEE_MUTEX_TRY_LOCK (&rrd_connection->mutex);
			if (unlocked) {
				cherokee_connection_sleep (conn, 200);
				return ret_eagain;
			}

			ret = render_srv_traffic (hdl, interval);
			if (ret != ret_ok) {
				TRACE (ENTRIES, "Couldn't render image: %s\n", conn->request.buf);

				CHEROKEE_MUTEX_UNLOCK (&rrd_connection->mutex);
				conn->error_code = http_service_unavailable;
				return ret_error;
			}

			CHEROKEE_MUTEX_UNLOCK (&rrd_connection->mutex);
		}


	} else if (! strncmp (begin, "/vserver_traffic_", 17)) {
		const char        *vserver_name;
		int                vserver_len;
		cherokee_buffer_t  vserver_buf   = CHEROKEE_BUF_INIT;

		/* Virtual server name
		 */
		vserver_name = begin + 17;
		vserver_len  = dash - vserver_name;

		if (vserver_len <= 0) {
			TRACE (ENTRIES, "Bad virtual server name. Length: %d\n", vserver_len);
			conn->error_code = http_service_unavailable;
			return ret_error;
		}

		cherokee_buffer_add (&vserver_buf, vserver_name, vserver_len);
		cherokee_buffer_replace_string (&vserver_buf, " ", 1, "_", 1);

		/* Is it fresh enough?
		 */
		cherokee_buffer_add_str    (&tmp, "vserver_traffic_");
		cherokee_buffer_add_buffer (&tmp, &vserver_buf);

		fresh = check_image_freshness (&tmp, interval);
		cherokee_buffer_mrproper (&tmp);

		if (! fresh) {
			unlocked = CHEROKEE_MUTEX_TRY_LOCK (&rrd_connection->mutex);
			if (unlocked) {
				cherokee_connection_sleep (conn, 200);
				return ret_eagain;
			}

			/* Render
			 */
			ret = render_vsrv_traffic (hdl, interval, &vserver_buf);
			cherokee_buffer_mrproper (&vserver_buf);

			if (ret != ret_ok) {
				TRACE (ENTRIES, "Couldn't render image: %s\n", conn->request.buf);

				CHEROKEE_MUTEX_UNLOCK (&rrd_connection->mutex);
				conn->error_code = http_service_unavailable;
				return ret_error;
			}

			CHEROKEE_MUTEX_UNLOCK (&rrd_connection->mutex);
		}

	} else {
		LOG_ERROR (CHEROKEE_ERROR_HANDLER_RENDER_RRD_INVALID_REQ, conn->request.buf);
		conn->error_code = http_service_unavailable;
		return ret_error;
	}

	/* Has everything gone alright?
	 */
	if (! cherokee_buffer_is_empty (&hdl->rrd_error)) {
		cherokee_connection_t *conn = HANDLER_CONN(hdl);

		conn->error_code = http_service_unavailable;
		BIT_SET (HANDLER(hdl)->support, hsupport_error);

		return ret_ok;
	}

	/* Rewrite the request
	 */
	if (cherokee_buffer_is_empty (&conn->request_original)) {
		cherokee_buffer_add_buffer (&conn->request_original, &conn->request);
	}

	cherokee_buffer_replace_string (&conn->request, " ", 1, "_", 1);

	/* Handler file init
	 */
	return cherokee_handler_file_init (hdl->file_hdl);
}


static ret_t
handler_add_headers (cherokee_handler_render_rrd_t *hdl,
                     cherokee_buffer_t             *buffer)
{
	if (! cherokee_buffer_is_empty (&hdl->rrd_error)) {
		cherokee_buffer_add_str (buffer, "Content-Type: text/html" CRLF);
		cherokee_buffer_add_va  (buffer, "Content-Length: %d" CRLF, hdl->rrd_error.len);
		return ret_ok;
	}

	if (HANDLER_RENDER_RRD_PROPS(hdl)->disabled) {
		cherokee_buffer_add_str (buffer, "Content-Type: text/html" CRLF);
		cherokee_buffer_add_va  (buffer, "Content-Length: %d" CRLF, strlen(DISABLED_MSG));
		return ret_ok;
	}

	return cherokee_handler_file_add_headers (hdl->file_hdl, buffer);
}


static ret_t
handler_step (cherokee_handler_render_rrd_t *hdl,
              cherokee_buffer_t             *buffer)
{
	if (! cherokee_buffer_is_empty (&hdl->rrd_error)) {
		cherokee_buffer_add_buffer (buffer, &hdl->rrd_error);
		return ret_eof_have_data;
	}

	if (HANDLER_RENDER_RRD_PROPS(hdl)->disabled) {
		cherokee_buffer_add_str (buffer, DISABLED_MSG);
		return ret_eof_have_data;
	}

	return cherokee_handler_file_step (hdl->file_hdl, buffer);
}


static ret_t
handler_free (cherokee_handler_render_rrd_t *hdl)
{
	cherokee_buffer_mrproper (&hdl->rrd_error);

	if (hdl->file_hdl != NULL) {
		cherokee_handler_free ((void *) hdl->file_hdl);
	}

	return ret_ok;
}


ret_t
cherokee_handler_render_rrd_new (cherokee_handler_t     **hdl,
                                 void                    *cnt,
                                 cherokee_module_props_t *props)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, handler_render_rrd);

	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(render_rrd));

	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_render_rrd_init;
	MODULE(n)->free         = (module_func_free_t) handler_free;
	HANDLER(n)->step        = (handler_func_step_t) handler_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) handler_add_headers;

	/* Supported features
	 */
	HANDLER(n)->support     = hsupport_nothing;

	/* Properties
	 */
	n->file_hdl = NULL;
	cherokee_buffer_init (&n->rrd_error);

	/* Instance file sub-handler
	 */
	if (PROP_RENDER_RRD(props)->disabled) {
		HANDLER(n)->support |= hsupport_length;

	} else {
		ret = cherokee_handler_file_new ((cherokee_handler_t **)&n->file_hdl, cnt, MODULE_PROPS(PROP_RENDER_RRD(props)->file_props));
		if (ret != ret_ok) {
			if (n->file_hdl) {
				cherokee_handler_free (HANDLER(n->file_hdl));
			}
			cherokee_handler_free (HANDLER(n));
			return ret_error;
		}

		HANDLER(n)->support = HANDLER(n->file_hdl)->support;
	}

	*hdl = HANDLER(n);
	return ret_ok;
}


static ret_t
props_free (cherokee_handler_render_rrd_props_t *props)
{
	if (props->file_props != NULL) {
		cherokee_handler_file_props_free (props->file_props);
	}

	return cherokee_handler_props_free_base (HANDLER_PROPS(props));
}


ret_t
cherokee_handler_render_rrd_configure (cherokee_config_node_t  *conf,
                                       cherokee_server_t       *srv,
                                       cherokee_module_props_t **_props)
{
	ret_t ret;

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_render_rrd_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n),
		                                  MODULE_PROPS_FREE(props_free));

		/* Sub-handler properties */
		n->disabled   = false;
		n->file_props = NULL;
		cherokee_handler_file_configure (conf, srv,
		                                 (cherokee_module_props_t **) &n->file_props);

		/* Force IO-cache off */
		n->file_props->use_cache = false;

		*_props = MODULE_PROPS(n);
	}

	/* Ensure the global RRD connection obj is ready
	 */
	cherokee_rrd_connection_get (NULL);

	/* Parse the configuration tree
	 */
	ret = cherokee_rrd_connection_configure (rrd_connection, conf);
	if (ret != ret_ok) {
		PROP_RENDER_RRD(*_props)->disabled = true;
		return ret_ok;
	}

	/* Ensure the image cache directory exists
	 */
	ret = cherokee_mkdir_p_perm (&rrd_connection->path_img_cache, 0775, W_OK);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_RRD_MKDIR_WRITE, rrd_connection->path_img_cache.buf);
		return ret_error;
	}

	TRACE(ENTRIES, "RRD cache image directory ready: %s\n", rrd_connection->path_img_cache.buf);
	return ret_ok;
}



/* Library init function
 */
static cherokee_boolean_t _render_rrd_is_init = false;

void
PLUGIN_INIT_NAME(render_rrd) (cherokee_plugin_loader_t *loader)
{
	if (_render_rrd_is_init) return;
	_render_rrd_is_init = true;

	/* Load the dependences
	 */
	cherokee_plugin_loader_load (loader, "file");
}
