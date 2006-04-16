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
#include "virtual_server.h"
#include "config_entry.h"
#include "list_ext.h"
#include "socket.h"
#include "reqs_list.h"
#include "server-protected.h"
#include "util.h"
#include "access.h"

#include "handler_error.h"

#include <errno.h>

#define ENTRIES "vserver"


ret_t 
cherokee_virtual_server_new (cherokee_virtual_server_t **vserver, void *server)
{
	ret_t ret;

       	CHEROKEE_NEW_STRUCT (n, virtual_server);

	INIT_LIST_HEAD (&n->list_entry);
	INIT_LIST_HEAD (&n->index_list);

	n->server_ref      = server;
	n->default_handler = NULL;
	n->error_handler   = NULL;
	n->logger          = NULL;
	n->logger_props    = NULL;

	ret = cherokee_virtual_entries_init (&n->entry);
	if (ret != ret_ok) return ret;

	ret = cherokee_virtual_entries_init (&n->userdir_entry);
	if (ret != ret_ok) return ret;

	n->data.rx         = 0;
	n->data.tx         = 0;
	CHEROKEE_MUTEX_INIT(&n->data.rx_mutex, NULL);
	CHEROKEE_MUTEX_INIT(&n->data.tx_mutex, NULL);

	n->server_cert     = NULL;
	n->server_key      = NULL;
	n->ca_cert         = NULL;

#ifdef HAVE_TLS
	ret = cherokee_table_init (&n->session_cache);
	if (unlikely(ret < ret_ok)) return ret;

# ifdef HAVE_GNUTLS
	n->credentials     = NULL;
# endif
# ifdef HAVE_OPENSSL
	n->context         = NULL;
# endif 
#endif

	ret = cherokee_buffer_init (&n->root);
	if (unlikely(ret < ret_ok)) return ret;	
	
	ret = cherokee_buffer_init (&n->name);
	if (unlikely(ret < ret_ok)) return ret;

	ret = cherokee_buffer_init (&n->userdir);
	if (unlikely(ret < ret_ok)) return ret;

	/* Return the object
	 */
	*vserver = n;
	return ret_ok;
}


ret_t 
cherokee_virtual_server_free (cherokee_virtual_server_t *vserver)
{
	if (vserver->server_cert != NULL) {
		free (vserver->server_cert);
		vserver->server_cert = NULL;
	}
	
	if (vserver->server_key != NULL) {
		free (vserver->server_key);
		vserver->server_key = NULL;
	}

	if (vserver->ca_cert != NULL) {
		free (vserver->ca_cert);
		vserver->ca_cert = NULL;
	}

	if (vserver->error_handler != NULL) {
		cherokee_config_entry_free (vserver->error_handler);
		vserver->error_handler = NULL;
	}

	if (vserver->default_handler != NULL) {
		cherokee_config_entry_free (vserver->default_handler);
		vserver->default_handler = NULL;
	}

#ifdef HAVE_TLS
	cherokee_table_mrproper (&vserver->session_cache);

# ifdef HAVE_GNUTLS
	if (vserver->credentials != NULL) {
		gnutls_certificate_free_credentials (vserver->credentials);
		vserver->credentials = NULL;
	}
# endif
# ifdef HAVE_OPENSSL
	if (vserver->context != NULL) {
		SSL_CTX_free (vserver->context);
		vserver->context = NULL;
	}
# endif
#endif

	cherokee_buffer_mrproper (&vserver->name);
	cherokee_buffer_mrproper (&vserver->root);

	if (vserver->logger != NULL) {
		cherokee_logger_free (vserver->logger);
		vserver->logger = NULL;
	}
	if (vserver->logger_props != NULL) {
		cherokee_table_free (vserver->logger_props);
		vserver->logger_props = NULL;
	}
	
	cherokee_buffer_mrproper (&vserver->userdir);

	/* Destroy the virtual_entries
	 */
	cherokee_virtual_entries_mrproper (&vserver->entry);
	cherokee_virtual_entries_mrproper (&vserver->userdir_entry);

	/* Index list
	 */
	cherokee_list_free (&vserver->index_list, free);

	free (vserver);	
	return ret_ok;
}


#ifdef HAVE_GNUTLS
static ret_t
generate_dh_params (gnutls_dh_params *dh_params)
{
	gnutls_dh_params_init (dh_params);
	gnutls_dh_params_generate2 (*dh_params, 1024);

	return ret_ok;
}

static ret_t
generate_rsa_params (gnutls_rsa_params *rsa_params)
{
	/* Generate RSA parameters - for use with RSA-export cipher
	 * suites. These should be discarded and regenerated once a
	 * day, once every 500 transactions etc. Depends on the
	 * security requirements.
	 */

	gnutls_rsa_params_init (rsa_params);
	gnutls_rsa_params_generate2 (*rsa_params, 512);

	return ret_ok;
}
#endif /* HAVE_GNUTLS */



ret_t 
cherokee_virtual_server_have_tls (cherokee_virtual_server_t *vserver)
{
#ifndef HAVE_TLS
	return ret_not_found;
#endif

	return ((vserver->server_cert != NULL) ||
		(vserver->server_key  != NULL) ||
		(vserver->ca_cert     != NULL)) ? ret_ok : ret_not_found;
}


ret_t 
cherokee_virtual_server_init_tls (cherokee_virtual_server_t *vsrv)
{
	int rc;

#ifndef HAVE_TLS
	return ret_ok;
#endif

	if ((vsrv->ca_cert     == NULL) &&
	    (vsrv->server_cert == NULL) &&
	    (vsrv->server_key  == NULL)) 
	{
		return ret_not_found;
	}

	if ((vsrv->ca_cert     == NULL) ||
	    (vsrv->server_cert == NULL) ||
	    (vsrv->server_key  == NULL)) 
	{
		return ret_error;
	}

#ifdef HAVE_GNUTLS
        rc = gnutls_certificate_allocate_credentials (&vsrv->credentials);
	if (rc < 0) {
		PRINT_ERROR_S ("ERROR: Couldn't allocate credentials.\n");
		return ret_error;
	}
	
	/* CA file
	 */
	rc = gnutls_certificate_set_x509_trust_file (vsrv->credentials,
						     vsrv->ca_cert,
						     GNUTLS_X509_FMT_PEM);
	if (rc < 0) {
		PRINT_ERROR ("ERROR: reading X.509 CA Certificate: '%s'\n", vsrv->ca_cert);
		return ret_error;
	}

	/* Key file
	 */
	rc = gnutls_certificate_set_x509_key_file (vsrv->credentials,
						   vsrv->server_cert,
						   vsrv->server_key,
						   GNUTLS_X509_FMT_PEM);	
	if (rc < 0) {
		PRINT_ERROR ("ERROR: reading X.509 key '%s' or certificate '%s' file\n", 
			     vsrv->server_key, vsrv->server_cert);
		return ret_error;
	}

	generate_dh_params (&vsrv->dh_params);
	generate_rsa_params (&vsrv->rsa_params);

	gnutls_certificate_set_dh_params (vsrv->credentials, vsrv->dh_params);
	gnutls_anon_set_server_dh_params (vsrv->credentials, vsrv->dh_params);
	gnutls_certificate_set_rsa_export_params (vsrv->credentials, vsrv->rsa_params);
#endif

#ifdef HAVE_OPENSSL
	/* Init the OpenSSL context
	 */
	vsrv->context = SSL_CTX_new (SSLv23_server_method());
	if (vsrv->context == NULL) {
		PRINT_ERROR_S("ERROR: OpenSSL: Couldn't allocate OpenSSL context\n");
		return ret_error;
	}

	/* Certificate
	 */
	rc = SSL_CTX_use_certificate_file (vsrv->context, vsrv->server_cert, SSL_FILETYPE_PEM);
	if (rc < 0) {
		char *error;

		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR("ERROR: OpenSSL: Can not use certificate file '%s':  %s\n", 
			    vsrv->server_cert, error);
		return ret_error;
	}

	/* Private key
	 */
	rc = SSL_CTX_use_PrivateKey_file (vsrv->context, vsrv->server_key, SSL_FILETYPE_PEM);
	if (rc < 0) {
		char *error;

		OPENSSL_LAST_ERROR(error);
		PRINT_ERROR("ERROR: OpenSSL: Can not use private key file '%s': %s\n", 
			    vsrv->server_key, error);
		return ret_error;
	}

	/* Check private key
	 */
	rc = SSL_CTX_check_private_key (vsrv->context);
	if (rc != 1) {
		PRINT_ERROR_S("ERROR: OpenSSL: Private key does not match the certificate public key\n");
		return ret_error;
	}
#endif

	return ret_ok;
}


void  
cherokee_virtual_server_add_rx (cherokee_virtual_server_t *vserver, size_t rx)
{
	CHEROKEE_MUTEX_LOCK(&vserver->data.rx_mutex);
	vserver->data.rx += rx;
	CHEROKEE_MUTEX_UNLOCK(&vserver->data.rx_mutex);
}


void
cherokee_virtual_server_add_tx (cherokee_virtual_server_t *vserver, size_t tx)
{
	CHEROKEE_MUTEX_LOCK(&vserver->data.tx_mutex);
	vserver->data.tx += tx;
	CHEROKEE_MUTEX_UNLOCK(&vserver->data.tx_mutex);
}


ret_t 
cherokee_virtual_server_set_documentroot (cherokee_virtual_server_t *vserver, const char *documentroot)
{
	cherokee_buffer_clean (&vserver->root);
	cherokee_buffer_add (&vserver->root, (char *)documentroot, strlen(documentroot));

	return ret_ok;
}


static ret_t 
add_directory_index (char *alias, void *data)
{
	cherokee_virtual_server_t *vserver = VSERVER(data);

	cherokee_list_add_tail (&vserver->index_list, strdup(alias));
	return ret_ok;
}

static ret_t
add_access (char *address, void *data)
{
	ret_t                    ret;
	cherokee_config_entry_t *entry = CONF_ENTRY(data);

	if (entry->access == NULL) {
		ret = cherokee_access_new ((cherokee_access_t **) &entry->access);
		if (ret != ret_ok) return ret;
	}

	ret = cherokee_access_add (entry->access, address);
	if (ret != ret_ok) return ret;

	return ret_ok;
}


static ret_t 
init_entry (cherokee_virtual_server_t *vserver, cherokee_config_node_t *config, cherokee_config_entry_t *entry)
{
	ret_t                      ret;
	int                        pri;
	cherokee_buffer_t         *tmp;
	cherokee_module_info_t    *info;
	cherokee_config_node_t    *subconf;

	/* Priority
	 */
	ret = cherokee_config_node_read_int (config, "priority", &pri);
	if (ret == ret_ok) {
		entry->priority = pri;
	}

	/* Handler
	 */
	ret = cherokee_config_node_get (config, "handler", &subconf);
	if (ret == ret_ok) {
		tmp = &subconf->val;

		ret = cherokee_module_loader_get (&SRV(vserver->server_ref)->loader, tmp->buf, &info);
		if (ret != ret_ok) return ret;

		if (info->configure) {
			info->configure (subconf, vserver->server_ref, &entry->handler_properties);
		}
		
		TRACE(ENTRIES, "Handler: %s\n", tmp->buf);
		cherokee_config_entry_set_handler (entry, info);		
	}

	/* Authentication
	 */
	ret = cherokee_config_node_get (config, "auth", &subconf);
	if (ret == ret_ok) {
		cherokee_module_info_validator_t *vinfo;
	   
		/* Load module
		 */
		tmp = &subconf->val;

		ret = cherokee_module_loader_get (&SRV(vserver->server_ref)->loader, tmp->buf, &info);
		if (ret != ret_ok) return ret;

		entry->validator_new_func = info->new_func;

		if (info->configure) {
			info->configure (subconf, vserver->server_ref, &entry->validator_properties);
		}

		/* Configure the entry
		 */
		vinfo = (cherokee_module_info_validator_t *)info;

		ret = cherokee_validator_configure (subconf, entry);
		if (ret != ret_ok) return ret;

		if ((entry->authentication & vinfo->valid_methods) != entry->authentication) {
			PRINT_MSG ("ERROR: '%s' unsupported methods\n", tmp->buf);
			return ret_error;
		}		

		TRACE(ENTRIES, "Validator: %s\n", tmp->buf);
	}

	/* DocumentRoot
	 */
	ret = cherokee_config_node_read_path (config, "document_root", &tmp);
	if (ret == ret_ok) {
		if (entry->document_root == NULL) 
			cherokee_buffer_new (&entry->document_root);
		else
			cherokee_buffer_clean (entry->document_root);

		TRACE(ENTRIES, "DocumentRoot: %s\n", tmp->buf);
		cherokee_buffer_add_buffer (entry->document_root, tmp);
	}

	/* Allow from
	 */
	ret = cherokee_config_node_get (config, "allow_from", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_config_node_read_list (subconf, NULL, add_access, entry);
		if (ret != ret_ok) return ret;
	}

	return ret_ok;
}

static ret_t 
add_directory (cherokee_config_node_t *config, cherokee_virtual_server_t *vserver, cherokee_virtual_entries_t *ventry)
{
	ret_t                      ret;
	cherokee_config_entry_t   *entry;
	char                      *dir     = config->key.buf;

	/* Create a new entry
	 */
	ret = cherokee_config_entry_new (&entry);
	if (unlikely (ret != ret_ok)) return ret;

	ret = init_entry (vserver, config, entry);
	if (ret != ret_ok) return ret;
	
	if (equal_buf_str (&config->key, "/")) {
		TRACE(ENTRIES, "Adding %s\n", "default directory");

		vserver->default_handler           = entry;
		vserver->default_handler->priority = CHEROKEE_CONFIG_PRIORITY_DEFAULT;
	} else {
		TRACE(ENTRIES, "Adding '%s' directory, priority %d\n", dir, entry->priority);

		ret = cherokee_dirs_table_add (&ventry->dirs, dir, entry);
		if (ret != ret_ok) {
			PRINT_MSG ("ERROR: Can't load handler '%s': Unknown error\n", dir);
			return ret;
		}

		cherokee_dirs_table_relink (&ventry->dirs);
	}

	return ret_ok;
}

static ret_t 
add_extensions (cherokee_config_node_t *config, cherokee_virtual_server_t *vserver, cherokee_virtual_entries_t *ventry)
{
	ret_t                      ret;
	char                      *end;
	cherokee_config_entry_t   *entry;
	char                      *ext = config->key.buf;

	ret = cherokee_config_entry_new (&entry);
	if (unlikely (ret != ret_ok)) return ret;

	ret = init_entry (vserver, config, entry);
	if (ret != ret_ok) return ret;

	if (vserver->entry.exts == NULL) {
		ret = cherokee_exts_table_new (&ventry->exts);
		if (ret != ret_ok) return ret;
	}

	for (;;) {
		end = strchr (ext, ',');
		if (end != NULL) *end = '\0';

		ret = cherokee_exts_table_has (ventry->exts, ext);
		if (ret != ret_not_found) {
			PRINT_MSG ("ERROR: Extension '%s' was already set\n", ext);
			return ret_error;
		}
	
		TRACE(ENTRIES, "Adding '%s' extension, priority %d\n", ext, entry->priority);
		
		ret = cherokee_exts_table_add (ventry->exts, ext, entry);
		if (ret != ret_ok) return ret;
		
		if (end == NULL)
			break;

		*end = ',';
		ext = end + 1;
	}

	return ret_ok;
}


static ret_t 
add_request (cherokee_config_node_t *config, cherokee_virtual_server_t *vserver, cherokee_virtual_entries_t *ventry)
{
	ret_t                       ret;
	cherokee_reqs_list_entry_t *entry;

	ret = cherokee_reqs_list_entry_new (&entry);
	if (ret != ret_ok) return ret;

	ret = init_entry (vserver, config, CONF_ENTRY(entry));
	if (ret != ret_ok) return ret;

	cherokee_buffer_add_buffer (&entry->request, &config->key);

	TRACE(ENTRIES, "Adding '%s' request, priority %d\n", config->key.buf, CONF_ENTRY(entry)->priority);

	ret = cherokee_reqs_list_add (&ventry->reqs, entry, SRV(vserver->server_ref)->regexs);
	if (ret != ret_ok) return ret;

	return ret_ok;
}


static ret_t 
configure_user_dir (cherokee_config_node_t *config, cherokee_virtual_server_t *vserver)
{
	ret_t                   ret;
	list_t                 *i;
	cherokee_config_node_t *subconf;

	cherokee_buffer_add_buffer (&vserver->userdir, &config->val);

	ret = cherokee_config_node_get (config, "directory", &subconf);
	if (ret == ret_ok) {
		cherokee_config_node_foreach (i, subconf) {
			add_directory (CONFIG_NODE(i), vserver, &vserver->userdir_entry);
		}
	}

	ret = cherokee_config_node_get (config, "extensions", &subconf);
	if (ret == ret_ok) {
		cherokee_config_node_foreach (i, subconf) {
			add_extensions (CONFIG_NODE(i), vserver, &vserver->userdir_entry);
		}
	}

	ret = cherokee_config_node_get (config, "request", &subconf);
	if (ret == ret_ok) {
		cherokee_config_node_foreach (i, subconf) {
			add_request (CONFIG_NODE(i), vserver, &vserver->userdir_entry);
		}
	}

	return ret_ok;
}


static ret_t 
configure_virtual_server_property (cherokee_config_node_t *conf, void *data)
{
	ret_t                      ret;
	cherokee_buffer_t         *tmp;
	list_t                    *i;
	char                      *key     = conf->key.buf;
	cherokee_virtual_server_t *vserver = VSERVER(data);

	if (equal_buf_str (&conf->key, "document_root")) {
		ret = cherokee_config_node_read_path (conf, NULL, &tmp);
		if (ret != ret_ok) return ret;
		
		ret = cherokee_virtual_server_set_documentroot (vserver, (const char *)tmp->buf);
		if (ret != ret_ok) {
			PRINT_MSG ("ERROR: Virtual server: Error setting DocumentRoot: '%s'\n", tmp->buf);
			return ret_error;
		}

	} else if (equal_buf_str (&conf->key, "user_dir")) {
		configure_user_dir (conf, vserver);

	} else if (equal_buf_str (&conf->key, "directory")) {
		cherokee_config_node_foreach (i, conf) {
			add_directory (CONFIG_NODE(i), vserver, &vserver->entry);
		}
	} else if (equal_buf_str (&conf->key, "extensions")) {
		cherokee_config_node_foreach (i, conf) {
			add_extensions (CONFIG_NODE(i), vserver, &vserver->entry);
		}
	} else if (equal_buf_str (&conf->key, "request")) {
		cherokee_config_node_foreach (i, conf) {
			add_request (CONFIG_NODE(i), vserver, &vserver->entry);
		}

	} else if (equal_buf_str (&conf->key, "directory_index")) {
		cherokee_config_node_read_list (conf, NULL, add_directory_index, vserver);
		
	} else if (equal_buf_str (&conf->key, "alias")) {
		/* Ignore it, server config already did this for us..
		 */
	} else {
		PRINT_MSG ("ERROR: Virtual Server: Unknown key '%s'\n", key);
		return ret_error;
	}
	
	return ret_ok;
}


ret_t 
cherokee_virtual_server_configure (cherokee_virtual_server_t *vserver, cherokee_buffer_t *name, cherokee_config_node_t *config)
{
	ret_t ret;

	/* Set vserver name
	 */
	if (cherokee_buffer_is_empty (&vserver->name)) {
		cherokee_buffer_add_buffer (&vserver->name, name);
	}

	/* Parse properties
	 */
	ret = cherokee_config_node_while (config, configure_virtual_server_property, vserver);
	if (ret != ret_ok) return ret;

	return ret_ok;
}
