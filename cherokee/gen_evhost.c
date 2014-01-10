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
#include "gen_evhost.h"
#include "plugin_loader.h"
#include "connection-protected.h"
#include "server-protected.h"
#include "util.h"

#define ENTRIES "evhost"

PLUGIN_INFO_EASY_INIT (cherokee_generic, evhost);
PLUGIN_EMPTY_INIT_FUNCTION(evhost);


static ret_t
_free (cherokee_generic_evhost_t *evhost)
{
	cherokee_template_mrproper (&evhost->tpl_document_root);

	free (evhost);
	return ret_ok;
}

static ret_t
_check_document_root (cherokee_connection_t *conn)

{
	ret_t                     ret;
	struct stat               stat_mem;
	struct stat              *info;
	cherokee_iocache_entry_t *io_entry = NULL;

	ret = cherokee_io_stat (CONN_SRV(conn)->iocache,
	                        &conn->local_directory,
	                        (CONN_SRV(conn)->iocache != NULL),
	                        &stat_mem, &io_entry, &info);

	if (ret != ret_ok) {
		ret = ret_not_found;
		goto out;
	}

	if (! S_ISDIR(info->st_mode)) {
		ret = ret_not_found;
		goto out;
	}

	ret = ret_ok;

out:
	if (io_entry) {
		cherokee_iocache_entry_unref (&io_entry);
	}
	return ret;
}

static ret_t
_render_document_root (cherokee_generic_evhost_t *evhost,
                       cherokee_connection_t     *conn)
{
	ret_t ret;

	/* Render the document root
	 */
	ret = cherokee_template_render (&evhost->tpl_document_root,
	                                &conn->local_directory, conn);
	if (unlikely (ret != ret_ok))
		return ret_error;

	if (! evhost->check_document_root)
		return ret_ok;

	/* Check the Document Root
	 */
	ret = _check_document_root (conn);
	if (ret != ret_ok) {
		TRACE(ENTRIES, "Dynamic Document Root '%s' doesn't exist\n", conn->local_directory.buf);
		return ret_not_found;
	}

	TRACE(ENTRIES, "Dynamic Document Root '%s' exists\n", conn->local_directory.buf);
	return ret_ok;
}

ret_t
cherokee_generic_evhost_configure (cherokee_generic_evhost_t *evhost,
                                   cherokee_config_node_t    *config)
{
	ret_t              ret;
	cherokee_buffer_t *tmp;

	cherokee_config_node_read_bool (config, "check_document_root",
	                                &evhost->check_document_root);

	ret = cherokee_config_node_read (config, "tpl_document_root", &tmp);
	if (ret != ret_ok) {
		LOG_CRITICAL_S (CHEROKEE_ERROR_GEN_EVHOST_TPL_DROOT);
		return ret;
	}

	ret = cherokee_template_parse (&evhost->tpl_document_root, tmp);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_GEN_EVHOST_PARSE, tmp->buf);
		return ret_error;
	}

	return ret_ok;
}


static ret_t
add_domain (cherokee_template_t       *template,
            cherokee_template_token_t *token,
            cherokee_buffer_t         *output,
            void                      *param)
{
	cherokee_connection_t *conn = CONN(param);
	UNUSED(template);
	UNUSED(token);
	return cherokee_buffer_add_buffer (output, &conn->host);
}


static ret_t
add_domain_md5 (cherokee_template_t       *template,
                cherokee_template_token_t *token,
                cherokee_buffer_t         *output,
                void                      *param)
{
	ret_t ret;
	cherokee_buffer_t *md5;
	cherokee_connection_t *conn = CONN(param);
	UNUSED(template);
	UNUSED(token);

	ret = cherokee_buffer_dup (&conn->host, &md5);
	if (unlikely (ret != ret_ok)) return ret;

	ret = cherokee_buffer_encode_md5_digest (md5);
	if (unlikely (ret != ret_ok)) return ret;

	ret = cherokee_buffer_add_buffer (output, md5);
	cherokee_buffer_mrproper (md5);

	return ret;
}


static ret_t
add_tld (cherokee_template_t       *template,
         cherokee_template_token_t *token,
         cherokee_buffer_t         *output,
         void                      *param)
{
	const char            *p;
	const char            *end;
	cherokee_connection_t *conn = CONN(param);

	UNUSED(template);
	UNUSED(token);

	end = conn->host.buf + conn->host.len;
	p   = end - 1;

	if (unlikely (*p == '.')) {
		/* Host: example.com. */
		return ret_deny;
	}

	while (p > conn->host.buf) {
		if (*p != '.') {
			p--;
			continue;
		}

		p += 1;
		cherokee_buffer_add (output, p, end-p);
		return ret_ok;
	}

	/* Host: example */
	return ret_not_found;
}

static ret_t
add_domain_no_tld (cherokee_template_t       *template,
                   cherokee_template_token_t *token,
                   cherokee_buffer_t         *output,
                   void                      *param)
{
	const char            *p;
	const char            *end;
	cherokee_connection_t *conn = CONN(param);

	UNUSED(template);
	UNUSED(token);

	end = conn->host.buf + conn->host.len;
	p   = end - 1;

	if (unlikely (*p == '.')) {
		/* Host: example.com. */
		return ret_deny;
	}

	while (p > conn->host.buf) {
		if (*p != '.') {
			p--;
			continue;
		}

		cherokee_buffer_add (output, conn->host.buf, p - conn->host.buf);
		return ret_ok;
	}

	/* Host: example */
	return ret_not_found;
}


static ret_t
_add_subdomain (cherokee_buffer_t     *output,
                cherokee_connection_t *conn,
                int                    starting_dot)
{
	const char *p;
	const char *end;
	const char *dom_end = NULL;

	end = conn->host.buf + conn->host.len;
	p   = end - 1;

	if (unlikely (*p == '.')) {
		/* Host: example.com. */
		return ret_deny;
	}

	while (p > conn->host.buf) {
		if (*p != '.') {
			p--;
			continue;
		}

		if (dom_end != NULL) {
			p++;
			break;
		}

		starting_dot--;
		if (starting_dot == 0) {
			dom_end = p;
		}

		p--;
	}

	if (dom_end != NULL) {
		cherokee_buffer_add (output, p, dom_end - p);
		return ret_ok;
	}

	/* Host: example */
	return ret_not_found;
}


static ret_t
add_root_domain (cherokee_template_t       *template,
                 cherokee_template_token_t *token,
                 cherokee_buffer_t         *output,
                 void                      *param)
{
	UNUSED(template);
	UNUSED(token);

	return _add_subdomain (output, CONN(param), 1);
}

static ret_t
add_subdomain1 (cherokee_template_t      *template,
                cherokee_template_token_t *token,
                cherokee_buffer_t         *output,
                void                      *param)
{
	UNUSED(template);
	UNUSED(token);

	return _add_subdomain (output, CONN(param), 2);
}

static ret_t
add_subdomain2 (cherokee_template_t      *template,
                cherokee_template_token_t *token,
                cherokee_buffer_t         *output,
                void                      *param)
{
	UNUSED(template);
	UNUSED(token);

	return _add_subdomain (output, CONN(param), 3);
}

ret_t
cherokee_generic_evhost_new (cherokee_generic_evhost_t **evhost)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT(n,generic_evhost);

	/* Methods
	 */
	cherokee_module_init_base (MODULE(n), NULL, PLUGIN_INFO_PTR(evhost));

	MODULE(n)->free        = (module_func_free_t) _free;
	n->func_document_root  = (evhost_func_droot_t) _render_document_root;
	n->check_document_root = true;

	/* Properties
	 */
	ret = cherokee_template_init (&n->tpl_document_root);
	if (ret != ret_ok) {
		return ret_error;
	}

	cherokee_template_set_token (&n->tpl_document_root, "domain",
	                             TEMPLATE_FUNC(add_domain), n, NULL);
	cherokee_template_set_token (&n->tpl_document_root, "domain_md5",
	                             TEMPLATE_FUNC(add_domain_md5), n, NULL);
	cherokee_template_set_token (&n->tpl_document_root, "tld",
	                             TEMPLATE_FUNC(add_tld), n, NULL);
	cherokee_template_set_token (&n->tpl_document_root, "domain_no_tld",
	                             TEMPLATE_FUNC(add_domain_no_tld), n, NULL);
	cherokee_template_set_token (&n->tpl_document_root, "root_domain",
	                             TEMPLATE_FUNC(add_root_domain), n, NULL);
	cherokee_template_set_token (&n->tpl_document_root, "subdomain1",
	                             TEMPLATE_FUNC(add_subdomain1), n, NULL);
	cherokee_template_set_token (&n->tpl_document_root, "subdomain2",
	                             TEMPLATE_FUNC(add_subdomain2), n, NULL);

	*evhost = n;
	return ret_ok;
}

