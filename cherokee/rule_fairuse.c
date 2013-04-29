/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Stefan de Konink <stefan@konink.de>
 *
 * Copyright (C) 2001-2013 Alvaro Lopez Ortega
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
#include "rule_fairuse.h"

#include "server-protected.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "rule,fairuse"

PLUGIN_INFO_RULE_EASIEST_INIT(fairuse);


static ret_t
match (cherokee_rule_fairuse_t *rule,
       cherokee_connection_t   *conn,
       cherokee_config_entry_t *ret_conf)
{
	ret_t ret;
    cherokee_socket_t *sock = &conn->socket;
    datum key, data;

	UNUSED(ret_conf);

#ifdef HAVE_IPV6
    /* This is a special case:
        * The socket is IPv6 with a mapped IPv4 address
        */
    if (SOCKET_AF(sock) == AF_INET6) {
        if (IN6_IS_ADDR_V4MAPPED (&SOCKET_ADDR_IPv6(sock)->sin6_addr)) {
            key.dptr = (void *) &SOCKET_ADDR_IPv6(sock)->sin6_addr.s6_addr[12];
            key.dsize = 4;
        } else {
            key.dptr = (void *) &SOCKET_ADDR_IPv6(sock)->sin6_addr;
            key.dsize = 16;
        }
    } else
#endif
    {
        key.dptr = (void *) &SOCKET_ADDR_IPv4(sock)->sin_addr;
        key.dsize = 4;
    }

    data = gdbm_fetch (rule->dbf, key);

    if (rule->count) {
        if (data.dsize > 0) {
            track_ip_t *tmp = (track_ip_t *) data.dptr;
            if (tmp->tm_yday == cherokee_bogonow_tmloc.tm_yday) {
                if (tmp->count > rule->limit) {
                    TRACE(ENTRIES, "Rule fairuse matched %s", "\n");
                
                    free(data.dptr);
                    return ret_ok;    
                } else {
                    tmp->count += 1;
                }
            } else {
                tmp->count = 1;
                tmp->tm_yday = cherokee_bogonow_tmloc.tm_yday;
            }

            gdbm_store (rule->dbf, key, data, GDBM_REPLACE);
            free(data.dptr);
        } else {
            track_ip_t store;
            store.count = 1;
            store.tm_yday = cherokee_bogonow_tmloc.tm_yday;
            data.dptr = (void *) &store;
            data.dsize = sizeof (track_ip_t);
            gdbm_store (rule->dbf, key, data, GDBM_INSERT);
        }
    } else {
        if (data.dsize > 0) {
            track_ip_t *tmp = (track_ip_t *) data.dptr;
            if (tmp->tm_yday == cherokee_bogonow_tmloc.tm_yday &&
                tmp->count > rule->limit) {
                TRACE(ENTRIES, "Rule fairuse matched %s", "\n");
                
                free(data.dptr);
                return ret_ok;
            }
            free(data.dptr);
        }
    }
	
    TRACE(ENTRIES, "Rule fairuse did %s match any\n", "not");
	return ret_not_found;
}

static ret_t
configure (cherokee_rule_fairuse_t   *rule,
           cherokee_config_node_t    *conf,
           cherokee_virtual_server_t *vsrv)
{
	ret_t                   ret;
	cherokee_list_t        *i;
	cherokee_config_node_t *subconf;

	UNUSED(vsrv);

	ret = cherokee_config_node_get (conf, "filename", &subconf);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_RULE_NO_PROPERTY,
			      RULE(rule)->priority, "filename");
		return ret_error;
	}

    rule->dbf = gdbm_open (subconf->val.buf, 0, GDBM_WRCREAT, 0666, 0);
    if (!rule->dbf) return ret_error;


    ret = cherokee_config_node_get (conf, "limit", &subconf);
    if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_RULE_NO_PROPERTY,
			      RULE(rule)->priority, "limit");
		return ret_error;
	}

    ret = cherokee_atoi (subconf->val.buf, &rule->limit);
    if (ret != ret_ok) return ret_error;

    
    ret = cherokee_config_node_get (conf, "count", &subconf);
    if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_RULE_NO_PROPERTY,
			      RULE(rule)->priority, "count");
		return ret_error;
	}

    ret = cherokee_atob (subconf->val.buf, &rule->count);
    if (ret != ret_ok) return ret_error;

	return ret_ok;
}

static ret_t
_free (void *p)
{
	cherokee_rule_fairuse_t *rule = RULE_FAIRUSE(p);

    if (rule->dbf)
        gdbm_close (rule->dbf);
	return ret_ok;
}


ret_t
cherokee_rule_fairuse_new (cherokee_rule_fairuse_t **rule)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, rule_fairuse);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(fairuse));

	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
    n->dbf = NULL;

	*rule = n;
 	return ret_ok;
}
