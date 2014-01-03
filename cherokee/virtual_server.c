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
#include "virtual_server.h"
#include "config_entry.h"
#include "socket.h"
#include "server-protected.h"
#include "util.h"
#include "access.h"
#include "handler_error.h"
#include "rule_default.h"
#include "gen_evhost.h"
#include "header_op.h"
#include "flcache.h"

#include <errno.h>

#define ENTRIES "vserver"
#define MAX_HOST_LEN 255


ret_t
cherokee_virtual_server_new (cherokee_virtual_server_t **vserver, void *server)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, virtual_server);

	INIT_LIST_HEAD (&n->list_node);
	INIT_LIST_HEAD (&n->index_list);

	n->server_ref      = server;
	n->default_handler = NULL;
	n->error_handler   = NULL;
	n->logger          = NULL;
	n->error_writer    = NULL;
	n->priority        = 0;
	n->keepalive       = true;
	n->cryptor         = NULL;
	n->post_max_len    = -1;
	n->evhost          = NULL;
	n->matching        = NULL;
	n->collector       = NULL;
	n->match_nick      = true;
	n->flcache         = NULL;

	n->hsts.enabled    = false;
	n->hsts.subdomains = true;
	n->hsts.max_age    = 365 * 24 * 60 * 60;

	n->cipher_server_preference = true;
	n->ssl_compression = false; /* This might prevent a SSL CRIME attack */
	n->ssl_dh_length   = 0;

	/* Virtual entries
	 */
	ret = cherokee_rule_list_init (&n->rules);
	if (ret != ret_ok)
		return ret;

	ret = cherokee_rule_list_init (&n->userdir_rules);
	if (ret != ret_ok)
		return ret;

	/* TLS related
	 */
	n->verify_depth = 1;
	cherokee_buffer_init (&n->server_cert);
	cherokee_buffer_init (&n->server_key);
	cherokee_buffer_init (&n->certs_ca);

	cherokee_buffer_init    (&n->ciphers);
	cherokee_buffer_add_str (&n->ciphers, CHEROKEE_CIPHERS_DEFAULT);

	ret = cherokee_buffer_init (&n->root);
	if (unlikely(ret < ret_ok))
		return ret;

	ret = cherokee_buffer_init (&n->name);
	if (unlikely(ret < ret_ok))
		return ret;

	ret = cherokee_buffer_init (&n->userdir);
	if (unlikely(ret < ret_ok))
		return ret;

	/* Return the object
	 */
	*vserver = n;
	return ret_ok;
}


ret_t
cherokee_virtual_server_free (cherokee_virtual_server_t *vserver)
{
	cherokee_buffer_mrproper (&vserver->server_cert);
	cherokee_buffer_mrproper (&vserver->server_key);
	cherokee_buffer_mrproper (&vserver->certs_ca);
	cherokee_buffer_mrproper (&vserver->ciphers);

	if (vserver->flcache != NULL) {
		cherokee_flcache_free (vserver->flcache);
		vserver->flcache = NULL;
	}

	if (vserver->error_handler != NULL) {
		cherokee_config_entry_free (vserver->error_handler);
		vserver->error_handler = NULL;
	}

	if (vserver->default_handler != NULL) {
		cherokee_config_entry_free (vserver->default_handler);
		vserver->default_handler = NULL;
	}

	if (vserver->cryptor != NULL) {
		cherokee_cryptor_vserver_free (vserver->cryptor);
		vserver->cryptor = NULL;
	}

	if (vserver->matching != NULL) {
		cherokee_vrule_free (vserver->matching);
		vserver->matching = NULL;
	}

	if (vserver->evhost != NULL) {
		MODULE(vserver->evhost)->free (vserver->evhost);
	}

	if (vserver->logger != NULL) {
		cherokee_logger_free (vserver->logger);
		vserver->logger = NULL;
	}

	if (vserver->collector != NULL) {
		cherokee_collector_vsrv_free (vserver->collector);
		vserver->collector = NULL;
	}

	cherokee_buffer_mrproper (&vserver->name);
	cherokee_buffer_mrproper (&vserver->root);
	cherokee_buffer_mrproper (&vserver->userdir);

	/* Destroy the virtual_entries
	 */
	cherokee_rule_list_mrproper (&vserver->rules);
	cherokee_rule_list_mrproper (&vserver->userdir_rules);

	/* Index list
	 */
	cherokee_list_content_free (&vserver->index_list,
	                            (cherokee_list_free_func) cherokee_buffer_free);

	free (vserver);
	return ret_ok;
}


ret_t
cherokee_virtual_server_has_tls (cherokee_virtual_server_t *vserver)
{
	if (! cherokee_buffer_is_empty (&vserver->server_cert))
		return ret_ok;
	if (! cherokee_buffer_is_empty (&vserver->server_key))
		return ret_ok;

	return ret_not_found;
}


ret_t
cherokee_virtual_server_init_tls (cherokee_virtual_server_t *vsrv)
{
	ret_t ret;
	cherokee_server_t *srv = VSERVER_SRV(vsrv);

	/* Check if all of them are empty
	 */
	if (cherokee_buffer_is_empty (&vsrv->server_cert) &&
	    cherokee_buffer_is_empty (&vsrv->server_key))
		return ret_not_found;

	/* Check if key or certificate are empty
	 */
	if (cherokee_buffer_is_empty (&vsrv->server_cert) ||
	    cherokee_buffer_is_empty (&vsrv->server_key))
		return ret_error;

	/* Instance virtual server's cryptor
	 */
	ret = cherokee_cryptor_vserver_new (srv->cryptor, vsrv, &vsrv->cryptor);
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}


static ret_t
add_directory_index (char *index, void *data)
{
	cherokee_buffer_t         *new_buf;
	cherokee_virtual_server_t *vserver = VSERVER(data);

	TRACE(ENTRIES, "Adding directory index '%s'\n", index);

	cherokee_buffer_new (&new_buf);
	cherokee_buffer_add (new_buf, index, strlen(index));

	cherokee_list_add_tail_content (&vserver->index_list, new_buf);
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
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}


static ret_t
add_header_op (cherokee_config_entry_t *entry,
               cherokee_config_node_t  *conf)
{
	ret_t                 ret;
	cherokee_header_op_t *op   = NULL;

	ret = cherokee_header_op_new (&op);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	ret = cherokee_header_op_configure (op, conf);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	if (entry->header_ops == NULL) {
		/* Create an empty list object under demand
		 */
		entry->header_ops = (cherokee_list_t *) malloc (sizeof (cherokee_list_t));
		if (unlikely (entry->header_ops == NULL)) {
			return ret_error;
		}
		INIT_LIST_HEAD (entry->header_ops);
	}

	cherokee_list_add (&op->entry, entry->header_ops);
	return ret_ok;
}


static ret_t
add_flcache_cookies_do_cache (cherokee_config_entry_t *entry,
                              cherokee_config_node_t  *conf,
                              cherokee_server_t       *srv)
{
	ret_t              ret;
	void              *pcre  = NULL;
	cherokee_buffer_t *regex = NULL;

	/* Get the property
	 */
	ret = cherokee_config_node_read (conf, "regex", &regex);
	if ((ret != ret_ok) || (regex == NULL)) {
		return ret_not_found;
	}

	/* Compile regular expression
	 */
	ret = cherokee_regex_table_add (srv->regexs, regex->buf);
	if (ret != ret_ok) {
		return ret_error;
	}

	ret = cherokee_regex_table_get (srv->regexs, regex->buf, &pcre);
	if ((ret != ret_ok) || (pcre == NULL)) {
		return ret_error;
	}

	/* Add it to the list
	 */
	if (entry->flcache_cookies_disregard == NULL) {
		entry->flcache_cookies_disregard = (cherokee_list_t *) malloc (sizeof (cherokee_list_t));
		if (unlikely (entry->flcache_cookies_disregard == NULL)) {
			return ret_error;
		}
		INIT_LIST_HEAD (entry->flcache_cookies_disregard);
	}

	TRACE (ENTRIES",flcache", "Flcache adding disregard cookie '%s'\n", regex->buf);
	cherokee_list_add_tail_content (entry->flcache_cookies_disregard, pcre);

	return ret_ok;
}


static ret_t
init_entry_property (cherokee_config_node_t *conf, void *data)
{
	ret_t                      ret;
	int                        val;
	cherokee_list_t           *i, *j;
	cherokee_buffer_t         *tmp       = NULL;
	cherokee_config_node_t    *subconf   = NULL;
	cherokee_config_node_t    *subconf2  = NULL;
	cherokee_plugin_info_t    *info      = NULL;
	cherokee_virtual_server_t *vserver   = ((void **)data)[0];
	cherokee_config_entry_t   *entry     = ((void **)data)[1];
	int                        rule_prio = POINTER_TO_INT(((void **)data)[2]);
	cherokee_server_t         *srv       = VSERVER_SRV(vserver);

	if (equal_buf_str (&conf->key, "allow_from")) {
		ret = cherokee_config_node_read_list (conf, NULL, add_access, entry);
		if (ret != ret_ok)
			return ret;

	} else if (equal_buf_str (&conf->key, "header_op")) {
		cherokee_config_node_foreach (i, conf) {
			ret = add_header_op (entry, CONFIG_NODE(i));
			if (ret != ret_ok)
				return ret;
		}

	} else if (equal_buf_str (&conf->key, "document_root")) {
		ret = cherokee_config_node_read_path (conf, NULL, &tmp);
		if (ret != ret_ok) {
			return ret_error;
		}

		if (entry->document_root == NULL)
			cherokee_buffer_new (&entry->document_root);
		else
			cherokee_buffer_clean (entry->document_root);

		cherokee_buffer_add_buffer (entry->document_root, tmp);
		cherokee_fix_dirpath (entry->document_root);

		TRACE(ENTRIES, "DocumentRoot: %s\n", entry->document_root->buf);

	} else if (equal_buf_str (&conf->key, "handler")) {
		tmp = &conf->val;

		ret = cherokee_plugin_loader_get (&srv->loader, tmp->buf, &info);
		if (ret != ret_ok)
			return ret;

		if (info->configure) {
			handler_func_configure_t configure = info->configure;

			ret = configure (conf, srv, &entry->handler_properties);
			if (ret != ret_ok)
				return ret;
		}

		TRACE(ENTRIES, "Handler: %s\n", tmp->buf);
		cherokee_config_entry_set_handler (entry, PLUGIN_INFO_HANDLER(info));

	} else if (equal_buf_str (&conf->key, "encoder")) {
		cherokee_config_node_foreach (i, conf)
		{
			cherokee_encoder_props_t *encoder_props = NULL;

			tmp = &CONFIG_NODE(i)->key;

			ret = cherokee_plugin_loader_get (&srv->loader, tmp->buf, &info);
			if (ret != ret_ok) {
				return ret;
			}

			if (info->configure) {
				encoder_func_configure_t configure = info->configure;

				ret = configure (CONFIG_NODE(i), srv, (cherokee_module_props_t **)&encoder_props);
				if (ret != ret_ok) {
					return ret;
				}

				encoder_props->instance_func = PLUGIN_INFO(info)->instance;
			}

			ret = cherokee_avl_get (&srv->encoders, tmp, NULL);
			if (ret != ret_ok) {
				cherokee_avl_add (&srv->encoders, tmp, info);
			}

			TRACE(ENTRIES, "Encoder: %s\n", tmp->buf);
			cherokee_config_entry_set_encoder (entry, tmp,
			                                   PLUGIN_INFO(info),
			                                   encoder_props);
		}

	} else if (equal_buf_str (&conf->key, "auth")) {
		cherokee_plugin_info_validator_t *vinfo;

		/* Load module
		 */
		tmp = &conf->val;

		ret = cherokee_plugin_loader_get (&srv->loader, tmp->buf, &info);
		if (ret != ret_ok)
			return ret;

		entry->validator_new_func = info->instance;

		if (info->configure) {
			validator_func_configure_t configure = info->configure;

			ret = configure (conf, srv, &entry->validator_properties);
			if (ret != ret_ok) return ret;
		}

		/* Configure the entry
		 */
		vinfo = (cherokee_plugin_info_validator_t *)info;

		ret = cherokee_validator_configure (conf, entry);
		if (ret != ret_ok)
			return ret;

		if ((entry->authentication & vinfo->valid_methods) != entry->authentication) {
			LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_BAD_METHOD,
			              tmp->buf, vserver->priority, rule_prio);
			return ret_error;
		}

		TRACE(ENTRIES, "Validator: %s\n", tmp->buf);

	} else if (equal_buf_str (&conf->key, "only_secure")) {
		ret = cherokee_atob (conf->val.buf, &entry->only_secure);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "flcache")) {
		/* Enable / Disable
		 */
		if ((equal_buf_str (&conf->val, "allow")) ||
		    (equal_buf_str (&conf->val, "1")))
		{
			entry->flcache = true;
		}
		else if ((equal_buf_str (&conf->val, "forbid")) ||
			 (equal_buf_str (&conf->val, "0")))
		{
			entry->flcache = false;
		}

		/* Policy
		 */
		subconf = NULL;
		ret = cherokee_config_node_get (conf, "policy", &subconf);
		if ((ret == ret_ok) && (subconf != NULL)) {
			if (equal_buf_str (&subconf->val, "explicitly_allowed")) {
				entry->flcache_policy = flcache_policy_explicitly_allowed;
			} else if (equal_buf_str (&subconf->val, "all_but_forbidden")) {
				entry->flcache_policy = flcache_policy_all_but_forbidden;
			} else {
				LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_FLCACHE_UNKNOWN_POLICY,
				              subconf->val.buf, vserver->priority, rule_prio);
				return ret_error;
			}
		}

		/* Cookies
		 */
		subconf = NULL;
		ret = cherokee_config_node_get (conf, "cookies", &subconf);
		if ((ret == ret_ok) && (subconf != NULL)) {

			/*  Cookies to disregard
			 */
			subconf2 = NULL;
			ret = cherokee_config_node_get (subconf, "do_cache", &subconf2);
			if ((ret == ret_ok) && (subconf2 != NULL)) {
				cherokee_list_t *i;

				cherokee_config_node_foreach (i, subconf2) {
					cherokee_config_node_t *child = CONFIG_NODE(i);

					ret = add_flcache_cookies_do_cache (entry, child, srv);
					if (ret != ret_ok)
						return ret;
				}
			}
		}

	} else if (equal_buf_str (&conf->key, "expiration")) {
		/* Expiration
		 */
		if (equal_buf_str (&conf->val, "none")) {
			entry->expiration = cherokee_expiration_none;

		} else if (equal_buf_str (&conf->val, "epoch")) {
			entry->expiration = cherokee_expiration_epoch;

		} else if (equal_buf_str (&conf->val, "max")) {
			entry->expiration = cherokee_expiration_max;

		} else if (equal_buf_str (&conf->val, "time")) {
			entry->expiration = cherokee_expiration_time;
			ret = cherokee_config_node_read (conf, "time", &tmp);
			if (ret != ret_ok) {
				LOG_ERROR (CHEROKEE_ERROR_VSERVER_TIME_MISSING,
				           vserver->priority, rule_prio);
				return ret_error;
			}

			entry->expiration_time = cherokee_eval_formated_time (tmp);
		}

		/* Caching policies
		 */
		cherokee_config_node_foreach (i, conf) {
			subconf = CONFIG_NODE(i);

			if (equal_buf_str (&subconf->key, "caching")) {
				if (equal_buf_str (&subconf->val, "public")) {
					BIT_SET (entry->expiration_prop, cherokee_expiration_prop_public);
				} else if (equal_buf_str (&subconf->val, "private")) {
					BIT_SET (entry->expiration_prop, cherokee_expiration_prop_private);
				} else if (equal_buf_str (&subconf->val, "no-cache")) {
					BIT_SET (entry->expiration_prop, cherokee_expiration_prop_no_cache);
				}

				cherokee_config_node_foreach (j, subconf) {
					cherokee_boolean_t active = false;

					subconf2 = CONFIG_NODE(j);

					if (equal_buf_str (&subconf2->key, "no-store")) {
						ret = cherokee_atob (subconf2->val.buf, &active);
						if ((ret == ret_ok) && (active)) {
							BIT_SET (entry->expiration_prop, cherokee_expiration_prop_no_store);
						}
					} else if (equal_buf_str (&subconf2->key, "no-transform")) {
						ret = cherokee_atob (subconf2->val.buf, &active);
						if ((ret == ret_ok) && (active)) {
							BIT_SET (entry->expiration_prop, cherokee_expiration_prop_no_transform);
						}
					} else if (equal_buf_str (&subconf2->key, "must-revalidate")) {
						ret = cherokee_atob (subconf2->val.buf, &active);
						if ((ret == ret_ok) && (active)) {
							BIT_SET (entry->expiration_prop, cherokee_expiration_prop_must_revalidate);
						}
					} else if (equal_buf_str (&subconf2->key, "proxy-revalidate")) {
						ret = cherokee_atob (subconf2->val.buf, &active);
						if ((ret == ret_ok) && (active)) {
							BIT_SET (entry->expiration_prop, cherokee_expiration_prop_proxy_revalidate);
						}
					}
				}
			}
		}

	} else if (equal_buf_str (&conf->key, "rate")) {
		ret = cherokee_atoi (conf->val.buf, &val);
		if (ret != ret_ok) return ret_error;
		entry->limit_bps = val;

	} else if (equal_buf_str (&conf->key, "no_log")) {
		if (equal_buf_str (&conf->val, "1")) {
			entry->no_log = true;
		}

	} else if (equal_buf_str (&conf->key, "timeout")) {
		ret = cherokee_atoi (conf->val.buf, &entry->timeout_lapse);
		if (ret != ret_ok) return ret_error;

		if (entry->timeout_header != NULL) {
			cherokee_buffer_free (entry->timeout_header);
		}
		cherokee_buffer_new (&entry->timeout_header);
		cherokee_buffer_add_va (entry->timeout_header, "Keep-Alive: timeout=%d"CRLF, entry->timeout_lapse);

	} else if (equal_buf_str (&conf->key, "match")) {
		/* Ignore: Previously handled
		 */

	} else if (equal_buf_str (&conf->key, "disabled")) {
		/* Ignore: Previously handled
		 */

	} else {
		LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_RULE_UNKNOWN_KEY,
		              conf->key.buf, vserver->priority, rule_prio);
		return ret_error;
	}

	return ret_ok;
}


static ret_t
init_entry (cherokee_virtual_server_t *vserver,
            cherokee_config_node_t    *config,
            int                        rule_prio,
            cherokee_config_entry_t   *entry)
{
	ret_t  ret;
	void  *params[3] = { vserver, entry, INT_TO_POINTER(rule_prio) };

	ret = cherokee_config_node_while (config, init_entry_property, (void *)params);
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}


static ret_t
add_error_handler (cherokee_config_node_t *config, cherokee_virtual_server_t *vserver)
{
	ret_t                    ret;
	cherokee_config_entry_t *entry = NULL;
	cherokee_plugin_info_t  *info  = NULL;
	cherokee_buffer_t       *name  = &config->val;

	ret = cherokee_config_entry_new (&entry);
	if (unlikely (ret != ret_ok))
		goto error;

	ret = cherokee_plugin_loader_get (&SRV(vserver->server_ref)->loader, name->buf, &info);
	if (ret != ret_ok)
		goto error;

	if (info->configure) {
		handler_func_configure_t configure = info->configure;

		ret = configure (config, vserver->server_ref, &entry->handler_properties);
		if (ret != ret_ok) {
			goto error;
		}
	}

	TRACE(ENTRIES, "Error handler: %s\n", name->buf);

	cherokee_config_entry_set_handler (entry, PLUGIN_INFO_HANDLER(info));
	vserver->error_handler = entry;

	return ret_ok;

error:
	if (entry) {
		cherokee_config_entry_free (entry);
	}
	return ret_error;
}


ret_t
cherokee_virtual_server_new_vrule (cherokee_virtual_server_t  *vserver,
                                   cherokee_config_node_t     *config,
                                   cherokee_vrule_t          **vrule)
{
	ret_t                   ret;
	vrule_func_new_t        func_new;
	cherokee_plugin_info_t *info      = NULL;
	cherokee_buffer_t      *type      = &config->val;
	cherokee_server_t      *srv       = VSERVER_SRV(vserver);

	/* Load plug-in
	 */
	TRACE (ENTRIES, "Loading %s\n", type->buf);

	ret = cherokee_plugin_loader_get (&srv->loader, type->buf, &info);
	if (ret < ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_LOAD_MODULE, type->buf, 0);
		return ret_error;
	}

	/* Instance the rule object
	 */
	func_new = (rule_func_new_t) info->instance;
	if (func_new == NULL)
		return ret_error;

	ret = func_new ((void **) vrule);
	if ((ret != ret_ok) || (*vrule == NULL))
		return ret_error;

	/* Configure it
	 */
	ret = cherokee_vrule_configure (*vrule, config, vserver);
	if (ret != ret_ok)
		return ret_error;

	return ret_ok;
}


ret_t
cherokee_virtual_server_new_rule (cherokee_virtual_server_t  *vserver,
                                  cherokee_config_node_t     *config,
                                  cuint_t                     priority,
                                  cherokee_rule_t           **rule)
{
	ret_t                   ret;
	rule_func_new_t         func_new;
	cherokee_plugin_info_t *info      = NULL;
	cherokee_buffer_t      *type      = &config->val;
	cherokee_server_t      *srv       = VSERVER_SRV(vserver);

	/* Sanity check
	 */
	if (cherokee_buffer_is_empty (type)) {
		LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_TYPE_MISSING, vserver->priority, priority);
		return ret_error;
	}

	TRACE (ENTRIES, "Adding type=%s\n", type->buf);

	/* Default is compiled in, the rest are loded as plug-ins
	 */
	if (equal_buf_str (type, "default")) {
		func_new = (rule_func_new_t) cherokee_rule_default_new;
	} else {
		ret = cherokee_plugin_loader_get (&srv->loader, type->buf, &info);
		if (ret < ret_ok) {
			LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_LOAD_MODULE, type->buf, priority);
			return ret_error;
		}

		func_new = (rule_func_new_t) info->instance;
	}

	/* Instance the rule object
	 */
	if (func_new == NULL)
		return ret_error;

	ret = func_new ((void **) rule);
	if ((ret != ret_ok) || (*rule == NULL))
		return ret_error;

	/* Configure it
	 */
	(*rule)->priority = priority;

	ret = cherokee_rule_configure (*rule, config, vserver);
	if (ret != ret_ok)
		return ret_error;

	return ret_ok;
}


static ret_t
add_rule (cherokee_config_node_t    *config,
          cherokee_virtual_server_t *vserver,
          cherokee_rule_list_t      *rule_list)
{
	ret_t                   ret;
	int                     tmp;
	cint_t                  prio;
	cherokee_rule_t        *rule    = NULL;
	cherokee_config_node_t *subconf = NULL;

	/* Validate priority
	 */
	ret = cherokee_atoi (config->key.buf, &prio);
	if (ret != ret_ok) return ret_error;

	if (prio <= CHEROKEE_RULE_PRIO_NONE) {
		LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_BAD_PRIORITY,
		              config->key.buf, vserver->priority);
		return ret_error;
	}

	/* Is disabled?
	 */
	ret = cherokee_config_node_get (config, "disabled", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_atoi (subconf->val.buf, &tmp);
		if ((ret == ret_ok) && (tmp)) {
			TRACE(ENTRIES, "Skipping rule '%s'\n", config->key.buf);
			return ret_ok;
		}
	}

	/* Configure the rule match section
	 */
	ret = cherokee_config_node_get (config, "match", &subconf);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_RULE_MATCH_MISSING,
		              vserver->priority, prio);
		return ret_error;
	}

	ret = cherokee_virtual_server_new_rule (vserver, subconf, prio, &rule);
	if (ret != ret_ok)
		goto failed;

	/* config_node -> config_entry
	 */
	ret = init_entry (vserver, config, prio, &rule->config);
	if (ret != ret_ok)
		goto failed;

	/* Add the rule to the vserver's list
	 */
	ret = cherokee_rule_list_add (rule_list, rule);
	if (ret != ret_ok)
		goto failed;

	return ret_ok;

failed:
	if (rule != NULL)
		cherokee_rule_free (rule);

	return ret_error;
}


static ret_t
configure_match (cherokee_config_node_t    *config,
                 cherokee_virtual_server_t *vserver)
{
	ret_t                   ret;
	vrule_func_new_t        func_new;
	cherokee_plugin_info_t *info      = NULL;
	cherokee_server_t      *srv       = SRV(vserver->server_ref);

	if (cherokee_buffer_is_empty (&config->val)) {
		LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_MATCH_MISSING, vserver->priority);
		return ret_error;
	}

	/* Instance a new virtual server match obj
	 */
	ret = cherokee_plugin_loader_get (&srv->loader, config->val.buf, &info);
	if (ret < ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_LOAD_MODULE,
		              config->val.buf, vserver->priority);
		return ret_error;
	}

	func_new = (vrule_func_new_t) info->instance;
	if (func_new == NULL)
		return ret_error;

	ret = func_new ((void **) &vserver->matching);
	if (ret != ret_ok)
		return ret;

	ret = cherokee_vrule_configure (vserver->matching, config, vserver);
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}


static ret_t
add_evhost (cherokee_config_node_t *config, cherokee_virtual_server_t *vserver)
{
	ret_t                   ret;
	evhost_func_new_t       func_new;
	evhost_func_configure_t func_config;
	cherokee_plugin_info_t *info         = NULL;
	cherokee_server_t      *srv          = SRV(vserver->server_ref);

	/* Ensure there is a logger to instance..
	 */
	if (cherokee_buffer_is_empty (&config->val)) {
		return ret_ok;
	}

	/* Load the plug-in
	 */
	ret = cherokee_plugin_loader_get (&srv->loader, config->val.buf, &info);
	if (ret < ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_LOAD_MODULE,
		              config->val.buf, vserver->priority);
		return ret_error;
	}

	func_new = (evhost_func_new_t) info->instance;
	if (func_new == NULL)
		return ret_error;

	func_config = (evhost_func_configure_t) info->configure;
	if (func_config == NULL)
		return ret_error;

	/* Instance the evhost object
	 */
	ret = func_new ((void **) &vserver->evhost, vserver);
	if (ret != ret_ok)
		return ret;

	/* Configure it
	 */
	ret = func_config (vserver->evhost, config);
	if (ret != ret_ok)
		return ret_error;

	return ret_ok;
}


static ret_t
add_error_writer (cherokee_config_node_t    *config,
                  cherokee_virtual_server_t *vserver)
{
	ret_t              ret;
	cherokee_server_t *srv = VSERVER_SRV(vserver);

	ret = cherokee_server_get_log_writer (srv, config, &vserver->error_writer);
	if (ret != ret_ok) {
		return ret_error;
	}

	TRACE (ENTRIES, "Added a virtual server error_writer, type=%d\n",
	       vserver->error_writer->type);

	return ret_ok;
}


static ret_t
add_hsts (cherokee_config_node_t    *config,
          cherokee_virtual_server_t *vserver)
{
	ret_t ret;

	ret = cherokee_atob (config->val.buf, &vserver->hsts.enabled);
	if (ret != ret_ok) return ret_error;

	cherokee_config_node_read_uint (config, "max_age",    &vserver->hsts.max_age);
	cherokee_config_node_read_bool (config, "subdomains", &vserver->hsts.subdomains);

	return ret_ok;
}


static ret_t
add_logger (cherokee_config_node_t    *config,
            cherokee_virtual_server_t *vserver)
{
	ret_t                   ret;
	logger_func_new_t       func_new;
	cherokee_plugin_info_t *info      = NULL;
	cherokee_server_t      *srv       = SRV(vserver->server_ref);

	/* Ensure there is a logger to instance..
	 */
	if (cherokee_buffer_is_empty (&config->val)) {
		return ret_ok;
	}

	/* Instance a new logger
	 */
	ret = cherokee_plugin_loader_get (&srv->loader, config->val.buf, &info);
	if (ret < ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_LOAD_MODULE,
		              config->val.buf, vserver->priority);
		return ret_error;
	}

	func_new = (logger_func_new_t) info->instance;
	if (func_new == NULL)
		return ret_error;

	ret = func_new ((void **) &vserver->logger, vserver, config);
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}


static ret_t
configure_rules (cherokee_config_node_t    *config,
                 cherokee_virtual_server_t *vserver,
                 cherokee_rule_list_t      *rule_list)
{
	ret_t                   ret;
	cherokee_list_t        *i;
	cherokee_config_node_t *subconf;
/*	cherokee_boolean_t      did_default = false; */

	cherokee_config_node_foreach (i, config) {
		subconf = CONFIG_NODE(i);

		ret = add_rule (subconf, vserver, rule_list);
		if (ret != ret_ok) return ret;
	}

	/* Sort rules by its priority
	 */
	cherokee_rule_list_sort (rule_list);

/* TODO: */
/* 	if (! did_default) { */
/* 		PRINT_ERROR ("ERROR: vserver '%s': A default rule is needed\n", vserver->name.buf); */
/* 		return ret_error; */
/* 	} */

	return ret_ok;
}

static ret_t
configure_user_dir (cherokee_config_node_t *config, cherokee_virtual_server_t *vserver)
{
	ret_t                   ret;
	cherokee_config_node_t *subconf;

	/* Set the user_dir directory. It must end by slash.
	 */
	cherokee_buffer_add_buffer (&vserver->userdir, &config->val);

	/* Configure the rest of the entries
	 */
	ret = cherokee_config_node_get (config, "rule", &subconf);
	if (ret == ret_ok) {
		ret = configure_rules (subconf, vserver, &vserver->userdir_rules);
		if (ret != ret_ok) return ret;
	}

	return ret_ok;
}


static ret_t
configure_virtual_server_property (cherokee_config_node_t *conf, void *data)
{
	ret_t                      ret;
	cherokee_buffer_t         *tmp;
	cherokee_virtual_server_t *vserver = VSERVER(data);

	if (equal_buf_str (&conf->key, "document_root")) {
		ret = cherokee_config_node_read_path (conf, NULL, &tmp);
		if (ret != ret_ok)
			return ret;

		cherokee_buffer_clean (&vserver->root);
		cherokee_buffer_add_buffer (&vserver->root, tmp);
		cherokee_fix_dirpath (&vserver->root);

	} else if (equal_buf_str (&conf->key, "match")) {
		ret = configure_match (conf, vserver);
		if (ret != ret_ok)
			return ret;

	} else if (equal_buf_str (&conf->key, "nick")) {
		cherokee_buffer_clean (&vserver->name);
		cherokee_buffer_add_buffer (&vserver->name, &conf->val);

	} else if (equal_buf_str (&conf->key, "keepalive")) {
		ret = cherokee_atob (conf->val.buf, &vserver->keepalive);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "user_dir")) {
		ret = configure_user_dir (conf, vserver);
		if (ret != ret_ok)
			return ret;

	} else if (equal_buf_str (&conf->key, "rule")) {
		ret = configure_rules (conf, vserver, &vserver->rules);
		if (ret != ret_ok) return ret;

	} else if (equal_buf_str (&conf->key, "error_handler")) {
		ret = add_error_handler (conf, vserver);
		if (ret != ret_ok)
			return ret;

	} else if (equal_buf_str (&conf->key, "evhost")) {
		ret = add_evhost (conf, vserver);
		if (ret != ret_ok)
			return ret_error;

	} else if (equal_buf_str (&conf->key, "logger")) {
		ret = add_logger (conf, vserver);
		if (ret != ret_ok)
			return ret;

	} else if (equal_buf_str (&conf->key, "error_writer")) {
		ret = add_error_writer (conf, vserver);
		if (ret != ret_ok)
			return ret;

	} else if (equal_buf_str (&conf->key, "hsts")) {
		ret = add_hsts (conf, vserver);
		if (ret != ret_ok)
			return ret;

	} else if (equal_buf_str (&conf->key, "directory_index")) {
		cherokee_config_node_read_list (conf, NULL, add_directory_index, vserver);

	} else if (equal_buf_str (&conf->key, "post_max_len")) {
		ret = cherokee_atoi (conf->val.buf, &vserver->post_max_len);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "nick")) {
		ret = cherokee_atob (conf->val.buf, &vserver->match_nick);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "ssl_verify_depth")) {
		ret = cherokee_atob (conf->val.buf, &vserver->verify_depth);
		if (ret != ret_ok) return ret_error;

	} else if (equal_buf_str (&conf->key, "ssl_certificate_file")) {
		cherokee_buffer_add_buffer (&vserver->server_cert, &conf->val);

	} else if (equal_buf_str (&conf->key, "ssl_certificate_key_file")) {
		cherokee_buffer_add_buffer (&vserver->server_key, &conf->val);

	} else if (equal_buf_str (&conf->key, "ssl_ca_list_file")) {
		cherokee_buffer_add_buffer (&vserver->certs_ca, &conf->val);

	} else if (equal_buf_str (&conf->key, "ssl_client_certs")) {
		if (cherokee_buffer_cmp_str (&conf->val, "skip") == 0) {
			vserver->req_client_certs = req_client_cert_skip;
		} else if (cherokee_buffer_cmp_str (&conf->val, "tolerate") == 0) {
			vserver->req_client_certs = req_client_cert_tolerate;
		} else if (cherokee_buffer_cmp_str (&conf->val, "accept") == 0) {
			vserver->req_client_certs = req_client_cert_accept;
		} else if (cherokee_buffer_cmp_str (&conf->val, "require") == 0) {
			vserver->req_client_certs = req_client_cert_require;
		} else {
			return ret_error;
		}

	} else if (equal_buf_str (&conf->key, "ssl_ciphers")) {
		cherokee_buffer_clean      (&vserver->ciphers);
		cherokee_buffer_add_buffer (&vserver->ciphers, &conf->val);

	} else if (equal_buf_str (&conf->key, "ssl_cipher_server_preference")) {
		ret = cherokee_atob (conf->val.buf, &vserver->cipher_server_preference);
		if (ret != ret_ok)
			return ret;

	} else if (equal_buf_str (&conf->key, "ssl_compression")) {
		ret = cherokee_atob (conf->val.buf, &vserver->ssl_compression);
		if (ret != ret_ok)
			return ret;

	} else if (equal_buf_str (&conf->key, "ssl_dh_length")) {
		ret = cherokee_atou (conf->val.buf, &vserver->ssl_dh_length);
		if (ret != ret_ok)
			return ret;

	} else if (equal_buf_str (&conf->key, "flcache") ||
		   equal_buf_str (&conf->key, "collector")) {
		/* Handled later on */

	} else if (equal_buf_str (&conf->key, "disabled")) {
		/* Ignore: Previously handled
		*/

	} else if (equal_buf_str (&conf->key, "collect_statistics")) {
		/* DEPRECATED: Ignore */
	} else {
		LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_UNKNOWN_KEY,
		              conf->key.buf, vserver->priority);
		return ret_error;
	}

	return ret_ok;
}

static ret_t
configure_collector (cherokee_virtual_server_t *vserver,
                     cherokee_config_node_t    *config)
{
	ret_t                 ret;
	cherokee_boolean_t    active    = true;
	cherokee_collector_t *collector = VSERVER_SRV(vserver)->collector;

	/* Read the value
	 */
	cherokee_config_node_read_bool (config, "collector!active", &active);

	/* Instance object if needed
	 */
	if (! active) {
		return ret_ok;
	}

	ret = cherokee_collector_vsrv_new (collector, config, &vserver->collector);
	if (ret != ret_ok) {
		return ret_error;
	}

	return ret_ok;
}

ret_t
cherokee_virtual_server_configure (cherokee_virtual_server_t *vserver,
                                   cuint_t                    prio,
                                   cherokee_config_node_t    *config)
{
	ret_t                   ret;
	int                     tmp;
	cherokee_list_t        *i;
	cherokee_config_node_t *subconf      = NULL;
	cherokee_boolean_t      uses_flcache = false;

	/* Set the priority
	 */
	vserver->priority = prio;

	/* Is disabled?
	*/
	ret = cherokee_config_node_get (config, "disabled", &subconf);
	if (ret == ret_ok) {
		ret = cherokee_atoi (subconf->val.buf, &tmp);
		if ((ret == ret_ok) && (tmp)) {
			TRACE(ENTRIES, "Skipping VServer '%s'\n", config->key.buf);
			return ret_deny;
		}
	}

	/* Parse properties
	 */
	ret = cherokee_config_node_while (config, configure_virtual_server_property, vserver);
	if (ret != ret_ok) {
		return ret;
	}

	/* Information collectors
	 */
	if (VSERVER_SRV(vserver)->collector) {
		ret = configure_collector (vserver, config);
		if (ret != ret_ok) {
			return ret_error;
		}
	}

	/* Front-line cache:
	 * Needs 'nick' to be set previously
	 */
	list_for_each (i, &vserver->rules.rules) {
		cherokee_rule_t *rule = RULE (list_entry(i, cherokee_rule_t, list_node));
		if (NULLB_TO_BOOL (rule->config.flcache)) {
			uses_flcache = true;
			break;
		}
	}

	if (uses_flcache) {
		ret = cherokee_flcache_new (&vserver->flcache);
		if (ret != ret_ok) {
			return ret;
		}

		subconf = NULL;
		cherokee_config_node_get (config, "flcache", &subconf);

		ret = cherokee_flcache_configure (vserver->flcache, subconf, vserver);
		if (ret != ret_ok) {
			return ret;
		}
	}

	/* Perform some sanity checks
	 */
	if (cherokee_buffer_is_empty (&vserver->name)) {
		LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_NICK_MISSING, prio);
		return ret_error;
	}

	if (cherokee_buffer_is_empty (&vserver->root)) {
		LOG_CRITICAL (CHEROKEE_ERROR_VSERVER_DROOT_MISSING, vserver->name.buf);
		return ret_error;
	}

	return ret_ok;
}


ret_t
cherokee_virtual_server_get_error_log (cherokee_virtual_server_t  *vserver,
                                       cherokee_logger_writer_t  **writer)
{
	/* Virtual server's custom error log
	 */
	if ((vserver->error_writer != NULL) &&
	    (vserver->error_writer->initialized))
	{
		*writer = vserver->error_writer;
		return ret_ok;
	}

	/* Server's error writer
	 */
	if ((VSERVER_SRV(vserver)->error_writer != NULL) &&
	    (VSERVER_SRV(vserver)->error_writer->initialized))
	{
		*writer = vserver->error_writer;
		return ret_ok;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}
