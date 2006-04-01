/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2006 Alvaro Lopez Ortega
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
#include "handler_server_info.h"

#ifdef HAVE_SYS_UTSNAME_H
# include <sys/utsname.h>
#endif

#include "util.h"
#include "connection.h"
#include "connection-protected.h"
#include "server.h"
#include "server-protected.h"
#include "module_loader.h"


cherokee_module_info_handler_t MODULE_INFO(server_info) = {
	.module.type     = cherokee_handler,                   /* type         */
	.module.new_func = cherokee_handler_server_info_new,   /* new func     */
	.valid_methods   = http_get                            /* http methods */
};


#define PAGE_HEADER                                                                                     \
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"DTD/xhtml1-transitional.dtd\">" CRLF\
"<html><header>"                                                                                    CRLF\
"<meta  HTTP-EQUIV=\"refresh\" CONTENT=\"30\"><meta http-equiv=Cache-Control content=no-cache>"     CRLF\
"<style type=\"text/css\"><!--"                                                                     CRLF\
"body {background-color: #ffffff; color: #000000;}"                                                 CRLF\
"body, td, th, h1, h2 {font-family: sans-serif;}"                                                   CRLF\
"pre {margin: 0px; font-family: monospace;}"                                                        CRLF\
"a:link {color: #000099; text-decoration: none; background-color: #ffffff;}"                        CRLF\
"a:hover {text-decoration: underline;}"                                                             CRLF\
"table {border-collapse: collapse;}"                                                                CRLF\
".center {text-align: center;}"                                                                     CRLF\
".center table { margin-left: auto; margin-right: auto; text-align: left;}"                         CRLF\
".center th { text-align: center !important; }"                                                     CRLF\
"td, th { border: 1px solid #000000; font-size: 75%; vertical-align: baseline;}"                    CRLF\
"h1 {font-size: 150%;}"                                                                             CRLF\
"h2 {font-size: 125%;}"                                                                             CRLF\
".p {text-align: left;}"                                                                            CRLF\
".e {background-color: #ccccff; font-weight: bold; color: #000000;}"                                CRLF\
".h {background-color: #9999cc; font-weight: bold; color: #000000;}"                                CRLF\
".v {background-color: #cccccc; color: #000000;}"                                                   CRLF\
"i {color: #666666; background-color: #cccccc;}"                                                    CRLF\
"img {float: right; border: 0px;}"                                                                  CRLF\
"hr {width: 600px; background-color: #cccccc; border: 0px; height: 1px; color: #000000;}"           CRLF\
"//--></style>"                                                                                     CRLF\
"<title>Cherokee Web Server Info</title></header>"                                                  CRLF\
"<body><div class=\"center\">"                                                                      CRLF\
"<table border=\"0\" cellpadding=\"3\" width=\"600\">"                                              CRLF\
"  <tr class=\"h\"><td>"                                                                            CRLF\
"    <a href=\"http://www.0x50.org/\"><img border=\"0\" src=\"?logo\" alt=\"Cherokee Logo\" /></a>" CRLF\
"    <h1 class=\"p\">Cherokee Version %s</h1>"                                                      CRLF\
"  </td></tr>"                                                                                      CRLF\
"</table><br />"

#define AUTHOR                                                                                          \
"<a href=\"http://www.alobbs.com\">Alvaro Lopez Ortega</a> &lt;alvaro@alobbs.com&gt;"

#define LICENSE                                                                                         \
"<p>Copyright (C) 2001, 2002, 2003, 2004, 2005 " AUTHOR "</p>"                                      CRLF\
"<p>This program is free software; you can redistribute it and/or"                                  CRLF\
"modify it under the terms of version 2 of the GNU General Public"                                  CRLF\
"License as published by the Free Software Foundation.</p>"                                         CRLF\
"<p>This program is distributed in the hope that it will be useful,"                                CRLF\
"but WITHOUT ANY WARRANTY; without even the implied warranty of"                                    CRLF\
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the"                                     CRLF\
"GNU General Public License for more details.</p>"                                                  CRLF\
"<p>You should have received a copy of the GNU General Public License"                              CRLF\
"along with this program; if not, write to the Free Software"                                       CRLF\
"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA</p>"

#define PAGE_FOOT                                                                                   CRLF\
"<h2>Cherokee License</h2>"                                                                         CRLF\
"<table border=\"0\" cellpadding=\"3\" width=\"600\">"                                              CRLF\
"<tr class=\"v\"><td>" LICENSE                                                                      CRLF\
"</td></tr>"                                                                                        CRLF\
"</table><br />"                                                                                    CRLF\
"</div></body></html>"                                                                             


static void
server_info_add_table (cherokee_buffer_t *buf, char *name, char *a_name, cherokee_buffer_t *content)
{
	cherokee_buffer_add_va (buf, "<h2><a name=\"%s\">%s</a></h2>", a_name, name);
	cherokee_buffer_add_str (buf, "<table border=\"0\" cellpadding=\"3\" width=\"600\">");
	cherokee_buffer_add_buffer (buf, content);
	cherokee_buffer_add_str (buf, "</table><br />");
}

static void
table_add_row_str (cherokee_buffer_t *buf, const char *name, const char *value)
{
	cherokee_buffer_add_va (buf, "<tr><td class=\"e\">%s</td><td class=\"v\">%s</td></tr>"CRLF, name, value);
}

static void
table_add_row_int (cherokee_buffer_t *buf, char *name, int value)
{
	cherokee_buffer_add_va (buf, "<tr><td class=\"e\">%s</td><td class=\"v\">%d</td></tr>"CRLF, name, value);
}

static void
table_add_row_offset (cherokee_buffer_t *buf, char *name, unsigned long int value)
{
	cherokee_buffer_add_va (buf, "<tr><td class=\"e\">%s</td><td class=\"v\">" FMT_OFFSET "</td></tr>"CRLF, name, value);
}

static void
add_uptime_row (cherokee_buffer_t *buf, cherokee_server_t *srv)
{
	unsigned int elapse = srv->bogo_now - srv->start_time;
	unsigned int days;
	unsigned int hours;
	unsigned int mins;

	cherokee_buffer_t *tmp;
	cherokee_buffer_new (&tmp);

	days = elapse / (60*60*24);
	elapse %= (60*60*24);

	hours = elapse / (60*60);
	elapse %= (60*60);
	
	mins = elapse / 60;
	elapse %= 60;

	if (days > 0) {
		cherokee_buffer_add_va (tmp, "%d Day%s, %d Hour%s, %d Minute%s, %d Seconds", 
					days, days>1?"s":"", hours, hours>1?"s":"", mins, mins>1?"s":"", elapse);
	} else if (hours > 0) {
		cherokee_buffer_add_va (tmp, "%d Hour%s, %d Minute%s, %d Seconds", 
					hours, hours>1?"s":"", mins, mins>1?"s":"", elapse);
	} else if (mins > 0) {
		cherokee_buffer_add_va (tmp, "%d Minute%s, %d Seconds", 
					mins, mins>1?"s":"", elapse);
	} else {
		cherokee_buffer_add_va (tmp, "%d Seconds", elapse);
	}
	
	table_add_row_str (buf, "Uptime", tmp->buf);
	cherokee_buffer_free (tmp);
}

static void
add_data_sent_row (cherokee_buffer_t *buf, cherokee_server_t *srv)
{
	size_t rx, tx;
	char tmp[5];

	cherokee_server_get_total_traffic (srv, &rx, &tx);
	
	cherokee_strfsize (tx, tmp);
	table_add_row_str (buf, "Data sent", tmp);

	cherokee_strfsize (rx, tmp);
	table_add_row_str (buf, "Data received", tmp);
}

static void
build_general_table_content (cherokee_buffer_t *buf, cherokee_server_t *srv)
{
	add_uptime_row (buf, srv);
	add_data_sent_row (buf, srv);
}

static void
build_server_table_content (cherokee_buffer_t *buf, cherokee_server_t *srv)
{
	const char *on  = "On";
	const char *off = "Off";

	table_add_row_int (buf, "Thread Number ", (srv->ncpus == -1) ? 1 : srv->ncpus);
	table_add_row_str (buf, "IPv6 ", (srv->ipv6 == 1) ? on : off);
	table_add_row_str (buf, "TLS support ", (srv->socket_tls != -1) ? on : off);
	table_add_row_int (buf, "TLS port ", srv->port_tls);
	table_add_row_str (buf, "Chroot ", (srv->chrooted) ? on : off);
	table_add_row_int (buf, "User ID", getuid());
	table_add_row_int (buf, "Group ID", getgid());

#ifdef HAVE_SYS_UTSNAME
	{
		int rc;
		struct utsname buf;

		rc = uname ((struct utsname *)&buf);
		if (rc >= 0) {
			table_add_row_str (buf, "Operating system", buf.sysname);
		}
	}
#endif

}

static void
build_connections_table_content (cherokee_buffer_t *buf, cherokee_server_t *srv)
{
	int active;
	int reusable;
	
	cherokee_server_get_active_conns (srv, &active);
	cherokee_server_get_reusable_conns (srv, &reusable);

	table_add_row_int (buf, "Active connections", active);
	table_add_row_int (buf, "Reusable connections", reusable);
}

static int
build_modules_table_content_while (const char *key, void *value, void *params[])
{
	int *loggers    = (int *) params[2];
	int *handlers   = (int *) params[3];
	int *encoders   = (int *) params[4];
	int *validators = (int *) params[5];
	int *generic    = (int *) params[6];

	cherokee_module_loader_entry_t *entry = value;
	cherokee_module_info_t         *mod   = entry->info;

	if (mod->type & cherokee_logger) {
		*loggers += 1;
	} else if (mod->type & cherokee_handler) {
		*handlers += 1;
	} else if (mod->type & cherokee_encoder) {
		*encoders += 1;
	} else if (mod->type & cherokee_validator) {
		*validators += 1;
	} else if (mod->type & cherokee_generic) {
		*generic += 1;
	} else {
		PRINT_ERROR("Unknown module type (%d)\n", mod->type);
	}

	return 1;
}

static void
build_modules_table_content (cherokee_buffer_t *buf, cherokee_server_t *srv)
{
	int   loggers    = 0;
	int   handlers   = 0;
	int   encoders   = 0;
	int   validators = 0;	   
	int   generic    = 0;
	void *params[]   = {buf, srv, &loggers, &handlers, &encoders, &validators, &generic};

	cherokee_table_while (&srv->loader, 
			      (cherokee_table_while_func_t) build_modules_table_content_while, 
			      params, NULL, NULL);

	table_add_row_int (buf, "Loggers", loggers);
	table_add_row_int (buf, "Handlers", handlers);
	table_add_row_int (buf, "Encoders",  encoders);
	table_add_row_int (buf, "Validators", validators);
	table_add_row_int (buf, "Generic", generic);
}

static ret_t
server_info_build_logo (cherokee_handler_server_info_t *hdl)
{
	ret_t ret;
	cherokee_buffer_t *buffer;

	buffer = &hdl->buffer;

#include "logo.inc"

	return ret_ok;
}

static void
build_icons_table_content (cherokee_buffer_t *buf, cherokee_server_t *srv)
{
	table_add_row_str (buf, "Default icon", (srv->icons->default_icon) ? srv->icons->default_icon : "");
	table_add_row_str (buf, "Directory icon", (srv->icons->directory_icon) ? srv->icons->directory_icon : "");
	table_add_row_str (buf, "Parent directory icon", (srv->icons->parentdir_icon) ? srv->icons->parentdir_icon : "");
}

static void
server_info_build_page (cherokee_handler_server_info_t *hdl)
{
	const char        *ver;
	cherokee_server_t *srv;
	cherokee_buffer_t *buf;
	cherokee_buffer_t *table;

	/* Init
	 */
	buf = &hdl->buffer;
	srv = HANDLER_SRV(hdl);
	   
	/* Add the page begining
	 */
	ver = (HANDLER_SRV(hdl)->server_token == cherokee_version_product) ?
		"" : PACKAGE_VERSION;

	cherokee_buffer_add_va (buf, PAGE_HEADER, ver);

	if (! hdl->just_about) {

		/* General table
		 */
		cherokee_buffer_new (&table);
		build_general_table_content (table, srv);
		server_info_add_table (buf, "General Information", "general", table);

		/* Server table
		 */
		cherokee_buffer_clean (table);
		build_server_table_content (table, srv);
		server_info_add_table (buf, "Server Core", "server_core", table);

		/* Connections table
		 */
		cherokee_buffer_clean (table);
		build_connections_table_content (table, srv);
		server_info_add_table (buf, "Current connections", "connections", table);

		/* Modules table
		 */
		cherokee_buffer_clean (table);
		build_modules_table_content (table, srv);
		server_info_add_table (buf, "Modules", "modules", table);

		/* Icons
		 */
		cherokee_buffer_clean (table);
		build_icons_table_content (table, srv);
		server_info_add_table (buf, "Icons", "icons", table);

		cherokee_buffer_free (table);
	}

	/* Add the page ending
	 */
	cherokee_buffer_add_str (buf, PAGE_FOOT);
}


ret_t
cherokee_handler_server_info_new  (cherokee_handler_t **hdl, cherokee_connection_t *cnt, cherokee_table_t *properties)
{
	CHEROKEE_NEW_STRUCT (n, handler_server_info);
	
	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt);
	   
	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_server_info_init;
	MODULE(n)->free         = (handler_func_free_t) cherokee_handler_server_info_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_server_info_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_server_info_add_headers;

	HANDLER(n)->support = hsupport_length | hsupport_range;

	/* Init
	 */
	n->just_about = false;

	cherokee_buffer_init (&n->buffer);
	cherokee_buffer_ensure_size (&n->buffer, 4*1024);

	if (properties) {
		cherokee_typed_table_get_int (properties, "about", &n->just_about);
	}

	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t 
cherokee_handler_server_info_free (cherokee_handler_server_info_t *hdl)
{
	cherokee_buffer_mrproper (&hdl->buffer);
	return ret_ok;
}


ret_t 
cherokee_handler_server_info_init (cherokee_handler_server_info_t *hdl)
{
	ret_t  ret;
	void  *param;
	int    web_interface = 1;

	cherokee_connection_parse_args (HANDLER_CONN(hdl));
		
	ret = cherokee_table_get (HANDLER_CONN(hdl)->arguments, "logo", &param);
	if (ret == ret_ok) {
		
		/* Build the logo
		 */
		server_info_build_logo (hdl);
		hdl->action = send_logo;
		
		return ret_ok;
	}
	
	/* Build the page
	 */
	if (web_interface) {
		server_info_build_page (hdl);
	}

	hdl->action = send_page;
	
	return ret_ok;
}


ret_t 
cherokee_handler_server_info_step (cherokee_handler_server_info_t *hdl, cherokee_buffer_t *buffer)
{
	cherokee_buffer_add_buffer (buffer, &hdl->buffer);
	return ret_eof_have_data;
}


ret_t 
cherokee_handler_server_info_add_headers (cherokee_handler_server_info_t *hdl, cherokee_buffer_t *buffer)
{
	cherokee_buffer_add_va (buffer, "Content-length: %d"CRLF, hdl->buffer.len);

	switch (hdl->action) {
	case send_logo:
		cherokee_buffer_add_str (buffer, "Content-Type: image/gif"CRLF);
		break;
	case send_page:
	default:
		cherokee_buffer_add_str (buffer, "Content-Type: text/html"CRLF);
		break;
	}

	return ret_ok;
}


/* Library init function
 */
static cherokee_boolean_t _server_info_is_init = false;

void
MODULE_INIT(server_info) (cherokee_module_loader_t *loader)
{
	/* Is init?
	 */
	if (_server_info_is_init) return;
	_server_info_is_init = true;
}
