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
#include "handler_server_info.h"
#include "bogotime.h"

#ifdef HAVE_SYS_UTSNAME_H
# include <sys/utsname.h>
#endif

#include "util.h"
#include "connection.h"
#include "connection-protected.h"
#include "server.h"
#include "server-protected.h"
#include "plugin_loader.h"
#include "connection_info.h"


#define PAGE_HEADER                                                                                     \
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"DTD/xhtml1-transitional.dtd\">" CRLF\
"<html><head>"                                                                                      CRLF\
"<title>Cherokee Web Server Info</title>"                                                           CRLF\
"<style type=\"text/css\">"                                                                         CRLF\
"body,div,dl,dt,dd,ul,ol,li,h1,h2,h3,h4,h5,h6,pre,code,p,th,td { margin:0;padding:0; }"             CRLF\
"table { border-collapse:collapse;border-spacing:0; }"                                              CRLF\
"img { border:0; }"                                                                                 CRLF\
"html { background-color: #cfd9e8; }"                                                               CRLF\
"body { min-width: 640px; font-family: helvetica, arial, sans-serif; font-size: small; "            CRLF\
"       text-align: center; background-color: #cfd9e8; }"                                           CRLF\
"a { color: #2d5e9a; }"                                                                             CRLF\
"a:hover { color: #164987; }"                                                                       CRLF\
"#container { background: #fff url({request}/logo.gif) 32px 16px no-repeat; "                       CRLF\
"              border: 1px solid #bacce2; width: 640px; min-width: 640px; "                         CRLF\
"              margin: 32px auto; text-align: left; }"                                              CRLF\
"#container-inner { padding: 32px 32px 32px 128px; }"                                               CRLF\
"h1 { color: #c00; font-weight: normal; font-size: 200%; margin-bottom: 0.5em; }"                   CRLF\
"h2 { color: #204a87; font-weight: bold; border-bottom: 2px solid #204a87; font-size: 120%; }"      CRLF\
"dl { margin-bottom: 24px; }"                                                                       CRLF\
"dt { width: 232px; float: left; clear: left; margin: 0; padding: 2px; height: 20px; "              CRLF\
"     text-align: right; border-bottom: 1px solid #bacce2; }"                                       CRLF\
"dt:after { content: \":\"; }"                                                                      CRLF\
"dd { width: 232px; margin: 0 0 0 232px; padding: 2px 8px; height: 20px; "                          CRLF\
"      border-bottom: 1px solid #bacce2; color: #555; }"                                            CRLF\
"#otherways { padding: 8px; border: 1px solid #bacce2; "                                            CRLF\
"             background-color: #eff6ff; margin-bottom: 24px; }"                                    CRLF\
"#license { padding: 16px; margin-bottom: 24px; }"                                                  CRLF\
"#license p { margin-bottom: 1em; }"                                                                CRLF\
"</style>"                                                                                          CRLF\
"<title>Cherokee Web Server Info</title></head>"                                                    CRLF\
"<body><div id=\"container\"><div id=\"container-inner\">"                                          CRLF\
"<h1>{cherokee_name}</h1>"                                                                          CRLF\
"<div id=\"information\"></div>"                                                                    CRLF\
"<div id=\"otherways\">"                                                                            CRLF\
"<p>The same information can also be fetched properly encoded to be consumed from: "                CRLF\
"<a href=\"{request}/info/py\">Python</a>, "                                                        CRLF\
"<a href=\"{request}/info/ruby\">Ruby</a>, "                                                        CRLF\
"<a href=\"{request}/info/js\">JavaScript</a> and "                                                 CRLF\
"<a href=\"{request}/info/php\">PHP</a>.</div>"

#define AJAX_JS                                                                                         \
"<script type=\"text/javascript\">"                                                                 CRLF\
"function ajaxObject (url) {"                                                                       CRLF\
"   var that = this;"                                                                               CRLF\
"   var updating = false;"                                                                          CRLF\
"   this.callback = function(txt) {}"                                                               CRLF\
"   this.update = function(passData) {"                                                             CRLF\
"      if (updating == true)"                                                                       CRLF\
"          return false;"                                                                           CRLF\
"      updating = true;"                                                                            CRLF\
"      var AJAX = null;"                                                                            CRLF\
"      if (window.XMLHttpRequest) {"                                                                CRLF\
"         AJAX = new XMLHttpRequest();"                                                             CRLF\
"      } else {"                                                                                    CRLF\
"         AJAX = new ActiveXObject('Microsoft.XMLHTTP');"                                           CRLF\
"      }"                                                                                           CRLF\
"      if (AJAX == null) {"                                                                         CRLF\
"         alert ('Your browser does not support AJAX.');"                                           CRLF\
"         return false"                                                                             CRLF\
"      } else {"                                                                                    CRLF\
"         AJAX.onreadystatechange = function() {"                                                   CRLF\
"            if (AJAX.readyState == 4 || AJAX.readyState == 'complete') {"                          CRLF\
"               delete AJAX;"                                                                       CRLF\
"               updating = false;"                                                                  CRLF\
"               that.callback(AJAX.responseText);"                                                  CRLF\
"            }"                                                                                     CRLF\
"         }"                                                                                        CRLF\
""                                                                                                  CRLF\
"         AJAX.open ('GET', url, true);"                                                            CRLF\
"         AJAX.send (null);"                                                                        CRLF\
"         return true;"                                                                             CRLF\
"      }"                                                                                           CRLF\
"   }"                                                                                              CRLF\
"} /* END: ajaxObject */"                                                                           CRLF\
"var info = {"                                                                                      CRLF\
"  uptime: {"                                                                                       CRLF\
"    title: 'Uptime',"                                                                              CRLF\
"    items: {"                                                                                      CRLF\
"      formatted: 'Uptime'"                                                                         CRLF\
"    }"                                                                                             CRLF\
"  },"                                                                                              CRLF\
"  connections: {"                                                                                  CRLF\
"    title: 'Current Connections',"                                                                 CRLF\
"    items: {"                                                                                      CRLF\
"      number: 'Number',"                                                                           CRLF\
"      active: 'Active', "                                                                          CRLF\
"      reusable: 'Reusable'"                                                                        CRLF\
"    }"                                                                                             CRLF\
"  },"                                                                                              CRLF\
"  config: {"                                                                                       CRLF\
"    title: 'Configuration',"                                                                       CRLF\
"    items: { "                                                                                     CRLF\
"      threads: 'Thread number',"                                                                   CRLF\
"      ipv6: 'IPv6',"                                                                               CRLF\
"      tls: 'SSL/TLS',"                                                                             CRLF\
"      chroot: 'Chroot jail',"                                                                      CRLF\
"      UID: 'UID',"                                                                                 CRLF\
"      GID: 'GID',"                                                                                 CRLF\
"      OS: 'Operating System'"                                                                      CRLF\
"    }"                                                                                             CRLF\
"  },"                                                                                              CRLF\
"  modules: {"                                                                                      CRLF\
"    title: 'Modules',"                                                                             CRLF\
"    items: {"                                                                                      CRLF\
"      loggers: 'Loggers',"                                                                         CRLF\
"      handlers: 'Handlers',"                                                                       CRLF\
"      encoders: 'Encoders',"                                                                       CRLF\
"      validators: 'Validators',"                                                                   CRLF\
"      generic: 'Generic', "                                                                        CRLF\
"      balancers: 'Balancers', "                                                                    CRLF\
"      rules: 'Rules',"                                                                             CRLF\
"      cryptors: 'Cryptors',"                                                                       CRLF\
"      vrules: 'VRules'"                                                                            CRLF\
"    }"                                                                                             CRLF\
"  },"                                                                                              CRLF\
"  icons: {"                                                                                        CRLF\
"    title: 'Icons',"                                                                               CRLF\
"    items: {"                                                                                      CRLF\
"      'default': 'Default',"                                                                       CRLF\
"      directory: 'Directory',"                                                                     CRLF\
"      parent: 'Parent'"                                                                            CRLF\
"    }"                                                                                             CRLF\
"  },"                                                                                              CRLF\
"  iocache: {"                                                                                      CRLF\
"    title: 'I/O cache',"                                                                           CRLF\
"    items: {"                                                                                      CRLF\
"      file_size_max_formatted: 'File Size: Max',"                                                  CRLF\
"      file_size_min_formatted: 'File Size: Min',"                                                  CRLF\
"      lasting_stat: 'Lasting: File information',"                                                  CRLF\
"      lasting_mmap: 'Lasting: File mapping', "                                                     CRLF\
"      size_max: 'Cache Max Size',"                                                                 CRLF\
"      fetches: 'Fetches',"                                                                         CRLF\
"      hits: 'Hits',"                                                                               CRLF\
"      misses: 'Misses', "                                                                          CRLF\
"      mmapped_formatted: 'Total Mapped' "                                                          CRLF\
"    }"                                                                                             CRLF\
"  }"                                                                                               CRLF\
"}"                                                                                                 CRLF\
"tmp = new ajaxObject ('{request}/info/js');"                                                       CRLF\
"tmp.callback = function(txt) {"                                                                    CRLF\
"  var data = null;"                                                                                CRLF\
"  var div = document.getElementById('information');"                                               CRLF\
"  eval('data = ' + txt);"                                                                          CRLF\
"  for (var section in info) {"                                                                     CRLF\
"    if ((data[section] === undefined) || (data[section] == null)) continue;"                       CRLF\
"    if (!document.getElementById(section)) {"                                                      CRLF\
"      h2 = document.createElement('h2');"                                                          CRLF\
"      h2.setAttribute('id', section);"                                                             CRLF\
"      h2.appendChild(document.createTextNode(info[section]['title']));"                            CRLF\
"      div.appendChild(h2);"                                                                        CRLF\
"    }"                                                                                             CRLF\
"    if (!(dl = document.getElementById('dl_' + section))) {"                                       CRLF\
"      dl = document.createElement('dl');"                                                          CRLF\
"      dl.setAttribute('id', 'dl_' + section);"                                                     CRLF\
"      div.appendChild(dl);"                                                                        CRLF\
"    } "                                                                                            CRLF\
"    for (var item in info[section]['items']) {"                                                    CRLF\
"      if ((data[section] == null) || (data[section][item] === undefined)) continue;"               CRLF\
"      if (!document.getElementById('dt_' + section + '_' + item)) { "                              CRLF\
"        dt = document.createElement('dt'); "                                                       CRLF\
"        dt.setAttribute('id', 'dt_' + section + '_' + item);"                                      CRLF\
"        dt.appendChild(document.createTextNode(info[section]['items'][item]));"                    CRLF\
"        dl.appendChild(dt);"                                                                       CRLF\
"      }"                                                                                           CRLF\
"      if (dd = document.getElementById('dd_' + section + '_' + item)) { "                          CRLF\
"        dd.innerHTML = data[section][item];"                                                       CRLF\
"      } else {"                                                                                    CRLF\
"        dd = document.createElement('dd'); "                                                       CRLF\
"        dd.setAttribute('id', 'dd_' + section + '_' + item);"                                      CRLF\
"        dd.appendChild(document.createTextNode(data[section][item]));"                             CRLF\
"        dl.appendChild(dd);"                                                                       CRLF\
"      }"                                                                                           CRLF\
"    }"                                                                                             CRLF\
"  }"                                                                                               CRLF\
"}"                                                                                                 CRLF\
"tmp.update(); "                                                                                    CRLF\
"setInterval('tmp.update()', 30000);"                                                               CRLF\
"</script>"

#define AUTHOR                                                                                          \
"<a href=\"http://www.alobbs.com/\">Alvaro Lopez Ortega</a> &lt;alvaro@alobbs.com&gt;"

#define LICENSE                                                                                         \
"<p>Copyright (C) 2001 - 2013 " AUTHOR "</p>"                                                       CRLF\
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
"<div id=\"license\">" LICENSE "</div>"                                                             CRLF\
"</div></div></body></html>"

#define LOGO_CACHING_TIME (60 *60 * 24)  /* 1 day */


/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (server_info, http_get);


/* Methods implementation
 */
static ret_t
props_free (cherokee_handler_server_info_props_t *props)
{
	return cherokee_module_props_free_base (MODULE_PROPS(props));
}


ret_t
cherokee_handler_server_info_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	cherokee_list_t                      *i;
	cherokee_handler_server_info_props_t *props;

	UNUSED(srv);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_server_info_props);

		cherokee_module_props_init_base (MODULE_PROPS(n),
		                                 MODULE_PROPS_FREE(props_free));
		n->just_about         = false;
		n->connection_details = false;

		*_props = MODULE_PROPS(n);
	}

	props = PROP_SRV_INFO(*_props);

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "type")) {
			if (equal_buf_str (&subconf->val, "normal")) {

			} else if (equal_buf_str (&subconf->val, "just_about")) {
				props->just_about = true;

			} else if (equal_buf_str (&subconf->val, "connection_details")) {
				props->connection_details = true;

			} else {
				LOG_ERROR (CHEROKEE_ERROR_HANDLER_SRV_INFO_TYPE, subconf->val.buf);
				return ret_error;
			}
		}
	}

	return ret_ok;
}


static void
add_uptime (cherokee_dwriter_t *writer, cherokee_server_t *srv)
{
	cuint_t           days;
	cuint_t           hours;
	cuint_t           mins;
	cherokee_buffer_t tmp   = CHEROKEE_BUF_INIT;
	cuint_t           lapse = cherokee_bogonow_now - srv->start_time;

	cherokee_dwriter_dict_open (writer);

	/* Raw seconds number */
	cherokee_dwriter_cstring (writer, "seconds");
	cherokee_dwriter_unsigned (writer, lapse);

	/* Formatted string */
	days = lapse / (60*60*24);
	lapse %= (60*60*24);
	hours = lapse / (60*60);
	lapse %= (60*60);
	mins = lapse / 60;
	lapse %= 60;

	if (days > 0) {
		cherokee_buffer_add_va (&tmp, "%d Day%s, %d Hour%s, %d Minute%s, %d Seconds",
		                        days, days>1?"s":"", hours, hours>1?"s":"", mins, mins>1?"s":"", lapse);
	} else if (hours > 0) {
		cherokee_buffer_add_va (&tmp, "%d Hour%s, %d Minute%s, %d Seconds",
		                        hours, hours>1?"s":"", mins, mins>1?"s":"", lapse);
	} else if (mins > 0) {
		cherokee_buffer_add_va (&tmp, "%d Minute%s, %d Seconds",
		                        mins, mins>1?"s":"", lapse);
	} else {
		cherokee_buffer_add_va (&tmp, "%d Seconds", lapse);
	}

	cherokee_dwriter_cstring (writer, "formatted");
	cherokee_dwriter_bstring (writer, &tmp);

	cherokee_dwriter_dict_close (writer);
	cherokee_buffer_mrproper (&tmp);
}


static void
add_traffic (cherokee_dwriter_t *writer, cherokee_server_t *srv)
{
	cherokee_list_t  *i;
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	/* Global statistics
	 */
	cherokee_dwriter_dict_open (writer);
	cherokee_dwriter_cstring (writer, "tx");
	if (srv->collector) {
		cherokee_dwriter_unsigned (writer, COLLECTOR_TX(srv->collector));
	} else {
		cherokee_dwriter_number (writer, "-1", 2);
	}

	cherokee_dwriter_cstring (writer, "rx");
	if (srv->collector) {
		cherokee_dwriter_unsigned (writer, COLLECTOR_RX(srv->collector));
	} else {
		cherokee_dwriter_number (writer, "-1", 2);
	}

	cherokee_dwriter_cstring (writer, "tx_formatted");
	if (srv->collector != NULL) {
		cherokee_buffer_clean     (&tmp);
		cherokee_buffer_add_fsize (&tmp, COLLECTOR_TX(srv->collector));
		cherokee_dwriter_bstring (writer, &tmp);
	} else {
		cherokee_dwriter_cstring (writer, "unknown");
	}

	cherokee_dwriter_cstring (writer, "rx_formatted");
	if (srv->collector != NULL) {
		cherokee_buffer_clean     (&tmp);
		cherokee_buffer_add_fsize (&tmp, COLLECTOR_RX(srv->collector));
		cherokee_dwriter_bstring (writer, &tmp);
	} else {
		cherokee_dwriter_cstring (writer, "unknown");
	}

	cherokee_dwriter_dict_open (writer);
	cherokee_dwriter_cstring (writer, "accepts");
	if (srv->collector) {
		cherokee_dwriter_unsigned (writer, COLLECTOR(srv->collector)->accepts);
	} else {
		cherokee_dwriter_number (writer, "-1", 2);
	}

	cherokee_dwriter_cstring (writer, "accepts_formatted");
	if (srv->collector != NULL) {
		cherokee_buffer_clean     (&tmp);
		cherokee_buffer_add_fsize (&tmp, COLLECTOR(srv->collector)->accepts);
		cherokee_dwriter_bstring (writer, &tmp);
	} else {
		cherokee_dwriter_cstring (writer, "unknown");
	}

	cherokee_dwriter_dict_open (writer);
	cherokee_dwriter_cstring (writer, "timeouts");
	if (srv->collector) {
		cherokee_dwriter_unsigned (writer, COLLECTOR(srv->collector)->timeouts);
	} else {
		cherokee_dwriter_number (writer, "-1", 2);
	}

	cherokee_dwriter_cstring (writer, "timeouts_formatted");
	if (srv->collector != NULL) {
		cherokee_buffer_clean     (&tmp);
		cherokee_buffer_add_fsize (&tmp, COLLECTOR(srv->collector)->timeouts);
		cherokee_dwriter_bstring (writer, &tmp);
	} else {
		cherokee_dwriter_cstring (writer, "unknown");
	}

	/* Per Virtual Server
	 */
	cherokee_dwriter_cstring (writer, "vservers");
	cherokee_dwriter_dict_open (writer);

	list_for_each (i, &srv->vservers) {
		cherokee_virtual_server_t *vsrv = VSERVER(i);

		cherokee_dwriter_bstring (writer, &vsrv->name);
		if (vsrv->collector != NULL) {
			cherokee_dwriter_dict_open (writer);
			cherokee_dwriter_cstring (writer, "rx");
			cherokee_dwriter_unsigned (writer, COLLECTOR_RX(vsrv->collector));
			cherokee_dwriter_cstring (writer, "tx");
			cherokee_dwriter_unsigned (writer, COLLECTOR_TX(vsrv->collector));
			cherokee_dwriter_dict_close (writer);
		} else {
			cherokee_dwriter_null (writer);
		}
	}

	cherokee_dwriter_dict_close (writer);
	cherokee_dwriter_dict_close (writer);

	/* Clean up
	 */
	cherokee_buffer_mrproper (&tmp);
}


static void
add_config (cherokee_dwriter_t *writer, cherokee_server_t *srv)
{
	cherokee_dwriter_dict_open (writer);

	cherokee_dwriter_cstring (writer, "threads");
	cherokee_dwriter_unsigned (writer, srv->thread_num);
	cherokee_dwriter_cstring (writer, "ipv6");
	cherokee_dwriter_bool    (writer, srv->ipv6);
	cherokee_dwriter_cstring (writer, "tls");
	cherokee_dwriter_bool    (writer, srv->tls_enabled);
	cherokee_dwriter_cstring (writer, "chroot");
	cherokee_dwriter_bool    (writer, srv->chrooted);
	cherokee_dwriter_cstring (writer, "UID");
	cherokee_dwriter_unsigned (writer, getuid());
	cherokee_dwriter_cstring (writer, "GID");
	cherokee_dwriter_unsigned (writer, getgid());

#ifdef HAVE_SYS_UTSNAME
	{
		int            rc;
		struct utsname buf;

		rc = uname ((struct utsname *)&buf);
		if (rc >= 0) {
			cherokee_dwriter_cstring (writer, "OS");
			cherokee_dwriter_string  (writer, buf.sysname, strlen(buf.sysname));
		}
	}
#endif

	cherokee_dwriter_dict_close (writer);
}


static void
add_connections (cherokee_dwriter_t *writer, cherokee_server_t *srv)
{
	cuint_t conns_num = 0;
	cuint_t active    = 0;
	cuint_t reusable  = 0;

	cherokee_server_get_conns_num (srv, &conns_num);
	cherokee_server_get_active_conns (srv, &active);
	cherokee_server_get_reusable_conns (srv, &reusable);

	cherokee_dwriter_dict_open (writer);
	cherokee_dwriter_cstring (writer, "number");
	cherokee_dwriter_unsigned (writer, conns_num);
	cherokee_dwriter_cstring (writer, "active");
	cherokee_dwriter_unsigned (writer, active);
	cherokee_dwriter_cstring (writer, "reusable");
	cherokee_dwriter_unsigned (writer, reusable);
	cherokee_dwriter_dict_close (writer);
}


static int
modules_while (cherokee_buffer_t *key, void *value, void *params[])
{
	int *loggers    = (int *) params[0];
	int *handlers   = (int *) params[1];
	int *encoders   = (int *) params[2];
	int *validators = (int *) params[3];
	int *generic    = (int *) params[4];
	int *balancer   = (int *) params[5];
	int *rules      = (int *) params[6];
	int *cryptors   = (int *) params[7];
	int *vrules     = (int *) params[8];
	int *collectors = (int *) params[9];

	cherokee_plugin_loader_entry_t *entry = value;
	cherokee_plugin_info_t         *mod   = entry->info;

	UNUSED(key);

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
	} else if (mod->type & cherokee_balancer) {
		*balancer += 1;
	} else if (mod->type & cherokee_rule) {
		*rules += 1;
	} else if (mod->type & cherokee_cryptor) {
		*cryptors += 1;
	} else if (mod->type & cherokee_vrule) {
		*vrules += 1;
	} else if (mod->type & cherokee_collector) {
		*collectors += 1;
	} else {
		LOG_ERROR (CHEROKEE_ERROR_HANDLER_SRV_INFO_MOD, mod->type);
	}

	return 0;
}

static void
add_modules (cherokee_dwriter_t *writer, cherokee_server_t *srv)
{
	cuint_t  loggers    = 0;
	cuint_t  handlers   = 0;
	cuint_t  encoders   = 0;
	cuint_t  validators = 0;
	cuint_t  generic    = 0;
	cuint_t  balancers  = 0;
	cuint_t  rules      = 0;
	cuint_t  cryptors   = 0;
	cuint_t  vrules     = 0;
	cuint_t  collectors = 0;
	void    *params[]   = {&loggers, &handlers, &encoders, &validators, &generic,
	                       &balancers, &rules, &cryptors, &vrules, &collectors};

	cherokee_avl_while (AVL_GENERIC (&srv->loader.table),
	                    (cherokee_avl_while_func_t) modules_while,
	                    params, NULL, NULL);

	cherokee_dwriter_dict_open (writer);
	cherokee_dwriter_cstring (writer, "loggers");
	cherokee_dwriter_unsigned (writer, loggers);
	cherokee_dwriter_cstring (writer, "handlers");
	cherokee_dwriter_unsigned (writer, handlers);
	cherokee_dwriter_cstring (writer, "encoders");
	cherokee_dwriter_unsigned (writer, encoders);
	cherokee_dwriter_cstring (writer, "validators");
	cherokee_dwriter_unsigned (writer, validators);
	cherokee_dwriter_cstring (writer, "generic");
	cherokee_dwriter_unsigned (writer, generic);
	cherokee_dwriter_cstring (writer, "balancers");
	cherokee_dwriter_unsigned (writer, balancers);
	cherokee_dwriter_cstring (writer, "rules");
	cherokee_dwriter_unsigned (writer, rules);
	cherokee_dwriter_cstring (writer, "cryptors");
	cherokee_dwriter_unsigned (writer, cryptors);
	cherokee_dwriter_cstring (writer, "vrules");
	cherokee_dwriter_unsigned (writer, vrules);
	cherokee_dwriter_cstring (writer, "collectors");
	cherokee_dwriter_unsigned (writer, collectors);
	cherokee_dwriter_dict_close (writer);
}


static ret_t
server_info_build_logo (cherokee_handler_server_info_t *hdl, cherokee_buffer_t *buffer)
{
	ret_t ret;
	UNUSED(hdl);

#include "logo.inc"
	return ret_ok;
}

static ret_t
server_info_build_html (cherokee_handler_server_info_t *hdl, cherokee_buffer_t *buffer)
{
	cherokee_buffer_t ver = CHEROKEE_BUF_INIT;

	cherokee_buffer_add_str (buffer, PAGE_HEADER);
	cherokee_buffer_add_str (buffer, AJAX_JS);

	cherokee_version_add (&ver, HANDLER_SRV(hdl)->server_token);
	cherokee_buffer_replace_string (buffer, "{cherokee_name}", 15, ver.buf, ver.len);
	cherokee_buffer_mrproper (&ver);

	cherokee_buffer_replace_string (buffer, "{request}", 9,
	                                HANDLER_CONN(hdl)->request.buf,
	                                HANDLER_CONN(hdl)->request.len);

	cherokee_buffer_add_str (buffer, PAGE_FOOT);
	return ret_ok;
}


static void
add_icons (cherokee_dwriter_t *writer, cherokee_server_t *srv)
{
	if (srv->icons == NULL) {
		cherokee_dwriter_null (writer);
		return;
	}

	cherokee_dwriter_dict_open (writer);
	cherokee_dwriter_cstring (writer, "default");
	cherokee_dwriter_bstring (writer, &srv->icons->default_icon);
	cherokee_dwriter_cstring (writer, "directory");
	cherokee_dwriter_bstring (writer, &srv->icons->directory_icon);
	cherokee_dwriter_cstring (writer, "parent");
	cherokee_dwriter_bstring (writer, &srv->icons->parentdir_icon);
	cherokee_dwriter_dict_close (writer);
}


static void
add_iocache (cherokee_dwriter_t *writer, cherokee_server_t *srv)
{
	float               percent;
	size_t              mmaped  = 0;
	cherokee_buffer_t   tmp_buf = CHEROKEE_BUF_INIT;
	cherokee_iocache_t *iocache = srv->iocache;

	if (iocache == NULL) {
		cherokee_dwriter_null (writer);
		return;
	}

	cherokee_dwriter_dict_open (writer);

	/* General parameters */
	cherokee_dwriter_cstring (writer, "file_size_max");
	cherokee_dwriter_unsigned (writer, iocache->max_file_size);
	cherokee_dwriter_cstring (writer, "file_size_min");
	cherokee_dwriter_unsigned (writer, iocache->min_file_size);

	cherokee_buffer_add_fsize (&tmp_buf, iocache->max_file_size);
	cherokee_dwriter_cstring (writer, "file_size_max_formatted");
	cherokee_dwriter_bstring (writer, &tmp_buf);

	cherokee_buffer_clean (&tmp_buf);
	cherokee_buffer_add_fsize (&tmp_buf, iocache->min_file_size);
	cherokee_dwriter_cstring (writer, "file_size_min_formatted");
	cherokee_dwriter_bstring (writer, &tmp_buf);

	cherokee_dwriter_cstring (writer, "lasting_mmap");
	cherokee_dwriter_unsigned (writer, iocache->lasting_mmap);

	cherokee_dwriter_cstring (writer, "lasting_stat");
	cherokee_dwriter_unsigned (writer, iocache->lasting_stat);

	cherokee_dwriter_cstring (writer, "size_max");
	cherokee_dwriter_unsigned (writer, CACHE(iocache)->max_size);

	cherokee_dwriter_cstring (writer, "fetches");
	cherokee_dwriter_unsigned (writer, CACHE(iocache)->count);

	/* Fetches */
	if (CACHE(iocache)->count == 0)
		percent = 0;
	else
		percent = (CACHE(iocache)->count_hit * 100.0) / CACHE(iocache)->count;
	cherokee_dwriter_cstring (writer, "hits");
	cherokee_dwriter_double  (writer, percent);

	/* Misses */
	if (CACHE(iocache)->count == 0)
		percent = 0;
	else
		percent = (CACHE(iocache)->count_miss * 100.0) / CACHE(iocache)->count;
	cherokee_dwriter_cstring (writer, "misses");
	cherokee_dwriter_double  (writer, percent);

	/* Total Mmaped */
	cherokee_iocache_get_mmaped_size (iocache, &mmaped);
	cherokee_dwriter_cstring (writer, "mmaped");
	cherokee_dwriter_unsigned (writer, mmaped);

	cherokee_buffer_clean (&tmp_buf);
	cherokee_buffer_add_fsize (&tmp_buf, mmaped);
	cherokee_dwriter_cstring (writer, "mmaped_formatted");
	cherokee_dwriter_bstring (writer, &tmp_buf);

	cherokee_dwriter_dict_close (writer);
	cherokee_buffer_mrproper (&tmp_buf);
}


static void
add_detailed_connections (cherokee_dwriter_t *writer, cherokee_list_t *infos)
{
	cherokee_list_t   *i, *j;
	cherokee_buffer_t tmp    = CHEROKEE_BUF_INIT;

	cherokee_dwriter_list_open (writer);

	list_for_each_safe (i, j, infos) {
		cherokee_connection_info_t *info = CONN_INFO(i);

		cherokee_dwriter_dict_open (writer);
		cherokee_dwriter_cstring (writer, "id");
		cherokee_dwriter_bstring (writer, &info->id);
		cherokee_dwriter_cstring (writer, "ip_remote");
		cherokee_dwriter_bstring (writer, &info->ip);
		cherokee_dwriter_cstring (writer, "phase");
		cherokee_dwriter_bstring (writer, &info->phase);
		cherokee_dwriter_cstring (writer, "request");
		cherokee_dwriter_bstring (writer, &info->request);
		cherokee_dwriter_cstring (writer, "handler");
		cherokee_dwriter_bstring (writer, &info->handler);
		cherokee_dwriter_cstring (writer, "percentage");
		if (!cherokee_buffer_is_empty (&info->percent)) {
			cherokee_dwriter_number  (writer,
			                          info->percent.buf, info->percent.len);
		} else {
			cherokee_dwriter_null (writer);
		}

		cherokee_dwriter_cstring (writer, "tx");
		cherokee_dwriter_number  (writer, info->tx.buf, info->tx.len);
		cherokee_dwriter_cstring (writer, "rx");
		cherokee_dwriter_number  (writer, info->rx.buf, info->rx.len);

		cherokee_buffer_clean (&tmp);
		cherokee_buffer_add_fsize (&tmp, strtoll(info->tx.buf, (char**)NULL, 10));
		cherokee_dwriter_cstring (writer, "tx_formatted");
		cherokee_dwriter_bstring (writer, &tmp);

		cherokee_buffer_clean (&tmp);
		cherokee_buffer_add_fsize (&tmp, strtoll(info->rx.buf, (char**)NULL, 10));
		cherokee_dwriter_cstring (writer, "rx_formatted");
		cherokee_dwriter_bstring (writer, &tmp);

		if (! cherokee_buffer_is_empty (&info->total_size)) {
			cherokee_buffer_clean (&tmp);
			cherokee_buffer_add_fsize (&tmp, strtoll(info->total_size.buf, (char**)NULL, 10));
			cherokee_dwriter_cstring (writer, "size");
			cherokee_dwriter_bstring (writer, &tmp);
		}

		if (! cherokee_buffer_is_empty (&info->icon)) {
			cherokee_dwriter_cstring (writer, "icon");
			cherokee_dwriter_bstring (writer, &info->icon);
		}

		cherokee_dwriter_dict_close (writer);
		cherokee_connection_info_free (info);
	}

	cherokee_dwriter_list_close (writer);
	cherokee_buffer_mrproper (&tmp);
}


static void
server_info_build_info (cherokee_handler_server_info_t *hdl)
{
	ret_t               ret;
	cherokee_dwriter_t *writer = &hdl->writer;
	cherokee_server_t  *srv    = HANDLER_SRV(hdl);
	cherokee_buffer_t   ver    = CHEROKEE_BUF_INIT;

	cherokee_dwriter_dict_open (writer);

	/* Version
	 */
	cherokee_version_add (&ver, HANDLER_SRV(hdl)->server_token);
	cherokee_dwriter_cstring (writer, "version");
	cherokee_dwriter_bstring (writer, &ver);
	cherokee_buffer_mrproper (&ver);

	/* Show only 'About..'?
	 */
	if (HDL_SRV_INFO_PROPS(hdl)->just_about)
		goto out;

	cherokee_dwriter_cstring (writer, "traffic");
	add_traffic (writer, srv);

	cherokee_dwriter_cstring (writer, "uptime");
	add_uptime (writer, srv);

	cherokee_dwriter_cstring (writer, "config");
	add_config (writer, srv);

	cherokee_dwriter_cstring (writer, "connections");
	add_connections (writer, srv);

	cherokee_dwriter_cstring (writer, "modules");
	add_modules (writer, srv);

	cherokee_dwriter_cstring (writer, "icons");
	add_icons (writer, srv);

	cherokee_dwriter_cstring (writer, "iocache");
	add_iocache (writer, srv);

	/* Connection details
	 */
	if  (HDL_SRV_INFO_PROPS(hdl)->connection_details) {
		cherokee_list_t infos;
		INIT_LIST_HEAD (&infos);

		ret = cherokee_connection_info_list_server (&infos, HANDLER_SRV(hdl), HANDLER(hdl));
		if (ret == ret_ok) {
			cherokee_dwriter_cstring (writer, "detailed_connections");
			add_detailed_connections (writer, &infos);
		}

	}

out:
	cherokee_dwriter_dict_close (writer);
}


ret_t
cherokee_handler_server_info_new  (cherokee_handler_t      **hdl,
                                   cherokee_connection_t    *cnt,
                                   cherokee_module_props_t  *props)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, handler_server_info);

	/* Init the base class object
	 */
	cherokee_handler_init_base(HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(server_info));

	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_server_info_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_server_info_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_server_info_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_server_info_add_headers;

	/* Supported features
	 */
	HANDLER(n)->support = hsupport_nothing;

	/* Init
	 */
	ret = cherokee_buffer_init (&n->buffer);
	if (unlikely(ret != ret_ok))
		goto error;

	ret = cherokee_buffer_ensure_size (&n->buffer, 4*1024);
	if (unlikely(ret != ret_ok))
		goto error;

	ret = cherokee_dwriter_init (&n->writer, &CONN_THREAD(cnt)->tmp_buf1);
	if (unlikely(ret != ret_ok))
		goto error;

	n->writer.pretty = true;
	cherokee_dwriter_set_buffer (&n->writer, &n->buffer);

	*hdl = HANDLER(n);
	return ret_ok;

error:
	cherokee_handler_free (HANDLER(n));
	return ret_error;
}


ret_t
cherokee_handler_server_info_free (cherokee_handler_server_info_t *hdl)
{
	cherokee_buffer_mrproper (&hdl->buffer);
	cherokee_dwriter_mrproper (&hdl->writer);
	return ret_ok;
}


ret_t
cherokee_handler_server_info_init (cherokee_handler_server_info_t *hdl)
{
	if (strstr (HANDLER_CONN(hdl)->request.buf, "/logo.gif")) {
		server_info_build_logo (hdl, &hdl->buffer);
		hdl->action = send_logo;

	} else if (strstr (HANDLER_CONN(hdl)->request.buf + 1, "/info")) {
		if (strstr (HANDLER_CONN(hdl)->request.buf, "/js")) {
			hdl->writer.lang = dwriter_json;
		} else if (strstr (HANDLER_CONN(hdl)->request.buf, "/py")) {
			hdl->writer.lang = dwriter_python;
		} else if (strstr (HANDLER_CONN(hdl)->request.buf, "/php")) {
			hdl->writer.lang = dwriter_php;
		} else if (strstr (HANDLER_CONN(hdl)->request.buf, "/ruby")) {
			hdl->writer.lang = dwriter_ruby;
		}

		hdl->action = send_info;
		server_info_build_info (hdl);

	} else {
		hdl->action = send_html;
		server_info_build_html (hdl, &hdl->buffer);
	}

	return ret_ok;
}


ret_t
cherokee_handler_server_info_step (cherokee_handler_server_info_t *hdl,
                                   cherokee_buffer_t              *buffer)
{
	ret_t ret;

	ret = cherokee_buffer_add_buffer (buffer, &hdl->buffer);
	if (unlikely (ret != ret_ok))
		return ret_error;

	return ret_eof_have_data;
}


ret_t
cherokee_handler_server_info_add_headers (cherokee_handler_server_info_t *hdl,
                                          cherokee_buffer_t              *buffer)
{
	cherokee_connection_t *conn = HANDLER_CONN(hdl);

	if (cherokee_connection_should_include_length(conn)) {
		HANDLER(hdl)->support |= hsupport_length;
		cherokee_buffer_add_va (buffer, "Content-Length: %d"CRLF, hdl->buffer.len);
	}

	switch (hdl->action) {
	case send_logo:
		cherokee_buffer_add_str (buffer, "Content-Type: image/png"CRLF);

		conn->expiration      = cherokee_expiration_time;
		conn->expiration_time = LOGO_CACHING_TIME;
		break;

	case send_info:
		conn->expiration = cherokee_expiration_epoch;

		switch (hdl->writer.lang) {
		case dwriter_json:
			cherokee_buffer_add_str (buffer, "Content-Type: application/json" CRLF);
			break;
		case dwriter_python:
			cherokee_buffer_add_str (buffer, "Content-Type: application/x-python" CRLF);
			break;
		case dwriter_php:
			cherokee_buffer_add_str (buffer, "Content-Type: application/x-php" CRLF);
			break;
		case dwriter_ruby:
			cherokee_buffer_add_str (buffer, "Content-Type: application/x-ruby" CRLF);
			break;
		default:
			SHOULDNT_HAPPEN;
		}
		break;

	case send_html:
	default:
		conn->expiration = cherokee_expiration_epoch;

		cherokee_buffer_add_str (buffer, "Content-Type: text/html"CRLF);
		break;
	}

	return ret_ok;
}
