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

/* Mini-Howto compile PHP5:
 *
 * $ ./configure ./configure --with-mysql=/usr/include/mysql/ --with-mysql-sock=/var/run/mysqld/mysqld.sock && make
 * $ sudo cp sapi/cgi/php /usr/lib/cgi-bin/php5
 */

#include "common-internal.h"
#include "handler_phpcgi.h"

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif 

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#include "module.h"
#include "plugin_loader.h"
#include "handler_cgi.h"
#include "connection.h"
#include "connection-protected.h"


static char *php_paths[] = {
#ifndef _WIN32
	"/usr/lib/cgi-bin/",
	"/usr/local/bin/",
	"/usr/bin/",
#else
	"C:/PHP/",
#endif
	NULL
};

static char *php_names[] = {
#ifndef _WIN32
	"php-cgi",
	"php", 
	"php5", 
	"php4", 
	"php-cgi",
#else
	"PHP.EXE",
#endif
	NULL
};


/* Plugin initialization
 */
PLUGIN_INFO_HANDLER_EASY_INIT (phpcgi, http_get | http_post | http_head);


static ret_t
check_interpreter (char *path)
{
	int re;
	
	/* Sanity check
	 */
	if (path == NULL)
		return ret_not_found;

	/* Check for the PHP executable 
	 */
	re = access (path, R_OK | X_OK);
	if (re != 0) {
		return ret_not_found;
	}

	return ret_ok;
}


static ret_t
search_php_executable (char **ret_path)
{
	cuint_t            npath;
	cuint_t            nname;
	cherokee_buffer_t  tmppath = CHEROKEE_BUF_INIT;

	for (npath = 0; php_paths[npath]; npath++) {
		for (nname = 0; php_names[nname]; nname++) {
			int re;

			cherokee_buffer_add_va (&tmppath, "%s%s", php_paths[npath], php_names[nname]);
			re = access (tmppath.buf, R_OK | X_OK);
			
			if (re == 0) {
				*ret_path = strdup (tmppath.buf);
				goto out;
			}

			cherokee_buffer_clean (&tmppath);
		}
	}

out:
	cherokee_buffer_mrproper (&tmppath);
	return ret_ok;
}


ret_t 
cherokee_handler_phpcgi_new  (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props)
{
	ret_t                        ret;
	cherokee_handler_cgi_base_t *cgi;
	char                        *interpreter = NULL;

	/* Create the new handler CGI object
	 */
	ret = cherokee_handler_cgi_new (hdl, cnt, props);
	if (unlikely(ret != ret_ok))
		return ret;

	cgi = HDL_CGI_BASE(*hdl);
	   
	/* Redefine the init method
	 */
	MODULE(*hdl)->init = (handler_func_init_t) cherokee_handler_phpcgi_init;

	/* Look for the interpreter in the properties
	 */
	if (props) {
		interpreter = PROP_PHPCGI(props)->interpreter.buf;
	}

	if (interpreter == NULL) 
		search_php_executable (&interpreter);

	/* Check the interpreter
	 */
	if (check_interpreter(interpreter) != ret_ok) {
		PRINT_ERROR ("ERROR: PHP interpreter not found (%s). Please install it.\n", 
			     interpreter ? interpreter : "");
		return ret_error;
	}

	/* Set it up in the CGI handler
	 */
	if (cgi->executable.len <= 0) {
		cherokee_buffer_add (&cgi->executable, interpreter, strlen(interpreter));
	}	
	
	/* If it has to fake the effective directory, set the -C paramter:
	 * Do not chdir to the script's directory
	 */
	if (!cherokee_buffer_is_empty (&CONN(cnt)->effective_directory)) {
		cherokee_handler_cgi_base_add_parameter (cgi, "-C", 2);
	}

	return ret_ok;
}


ret_t 
cherokee_handler_phpcgi_init (cherokee_handler_t *hdl)
{
	cherokee_handler_cgi_base_t *cgi  = HDL_CGI_BASE(hdl);
	cherokee_connection_t       *conn = HANDLER_CONN(hdl);
	cherokee_buffer_t           *ld   = &conn->local_directory;

	/* Special case:
	 * The CGI handler could return a ret_eagain value, so the connection
	 * will keep trying call this funcion.  The right action on this case
	 * is to call again the CGI handler
	 */
	if (cgi->init_phase != hcgi_phase_build_headers) {
		return cherokee_handler_cgi_init (HDL_CGI(hdl));
	}

	/* Add parameter to CGI handler
	*/
	if (cgi->param.len <= 0) {		
		cherokee_buffer_add_buffer (&cgi->param, ld);
		cherokee_buffer_add_buffer (&cgi->param, &conn->request);
		cherokee_handler_cgi_base_split_pathinfo (cgi, &cgi->param, ld->len, false);
	}
	
	cherokee_handler_cgi_add_env_pair (cgi, "REDIRECT_STATUS", 15, "200", 3); 
	cherokee_handler_cgi_add_env_pair (cgi, "SCRIPT_FILENAME", 15, cgi->param.buf, cgi->param.len);	

	return cherokee_handler_cgi_init (HDL_CGI(hdl));
}


static ret_t 
props_free (cherokee_handler_phpcgi_props_t *props)
{
	cherokee_buffer_mrproper (&props->interpreter);
	return cherokee_module_props_free_base (MODULE_PROPS(props));
}


ret_t 
cherokee_handler_phpcgi_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	cherokee_list_t                 *i;
	cherokee_handler_phpcgi_props_t *props;

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT(n, handler_phpcgi_props);

		cherokee_module_props_init_base (MODULE_PROPS(n), 
						 MODULE_PROPS_FREE(props_free));		

		cherokee_buffer_init (&n->interpreter);
		*_props = MODULE_PROPS(n);
	}

	props = PROP_PHPCGI(*_props);

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		if (equal_buf_str (&subconf->key, "interpreter")) {
			cherokee_buffer_add_buffer (&props->interpreter, &subconf->val);
		}
	}

	return cherokee_handler_cgi_configure (conf, srv, _props);
}


/*   Library init function
 */
static cherokee_boolean_t _phpcgi_is_init = false;

void  
PLUGIN_INIT_NAME(phpcgi) (cherokee_plugin_loader_t *loader)
{
	/* Is init?
	 */
	if (_phpcgi_is_init)
		return;
	_phpcgi_is_init = true;
	
	/* Load the dependences
	 */
	cherokee_plugin_loader_load (loader, "cgi");
}

