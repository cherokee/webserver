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
#include "rule_geoip.h"
#include "plugin_loader.h"
#include "virtual_server.h"
#include "socket.h"
#include "connection-protected.h"
#include "util.h"

#define ENTRIES "rule,geoip"
#define MAGIC   0XDEADBEEF

PLUGIN_INFO_RULE_EASIEST_INIT(geoip);



/* Specific plug-in functions
 */

static GeoIP   *_geoip      = NULL;
static cuint_t  _geoip_refs = 0;

static GeoIP *
geoip_get (void)
{
	int    i;
	GeoIP *gi;

	/* Return the global if it's ready
	 */
	if (likely (_geoip != NULL)) {
		_geoip_refs += 1;
		return _geoip;
	}

	/* Try to open a GeoIP database
	 */
	for (i = 0; i < NUM_DB_TYPES; ++i) {
		if (GeoIP_db_avail(i)) {
			gi = GeoIP_open_type (i, GEOIP_STANDARD);
			if (gi != NULL) {
				_geoip = gi;
				_geoip_refs += 1;

				return _geoip;
			}
		}
	}

	return NULL;
}

static void
geoip_release (void)
{
	_geoip_refs -= 1;

	if (_geoip_refs <= 0) {
		GeoIP_delete (_geoip);
		_geoip = NULL;
	}
}


/* The rule itself
 */

static ret_t
match (cherokee_rule_t         *rule_,
       cherokee_connection_t   *conn,
       cherokee_config_entry_t *ret_conf)
{
	ret_t                  ret;
	void                  *foo;
	const char            *country;
	cherokee_rule_geoip_t *rule = RULE_GEOIP(rule_);

	UNUSED(ret_conf);

	country = GeoIP_country_code_by_ipnum (rule->geoip, SOCKET_ADDRESS_IPv4(&conn->socket));
	if (country == NULL) {
		TRACE(ENTRIES, "Rule geoip did found the country for ip='%lu'\n",
		      SOCKET_ADDRESS_IPv4(&conn->socket));
		return ret_not_found;
	}

	ret = cherokee_avl_get_ptr (&rule->countries, country, &foo);
	if (ret != ret_ok) {
		TRACE(ENTRIES, "Rule geoip did not match '%s'\n", country);
		return ret;
	}

	TRACE(ENTRIES, "Match geoip did match: '%s'\n", country);
	return ret_ok;
}


static ret_t
parse_contry_list (cherokee_buffer_t *value, cherokee_avl_t *countries)
{
	char              *val;
	char              *tmpp;
	cherokee_buffer_t  tmp = CHEROKEE_BUF_INIT;

	TRACE(ENTRIES, "Adding geoip countries: '%s'\n", value->buf);
	cherokee_buffer_add_buffer (&tmp, value);

	tmpp = tmp.buf;
	while ((val = strsep(&tmpp, ",")) != NULL) {
		TRACE(ENTRIES, "Adding country: '%s'\n", val);
		cherokee_avl_add_ptr (countries, val, (void *)MAGIC);
	}

	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}


static ret_t
configure (cherokee_rule_geoip_t       *rule,
           cherokee_config_node_t    *conf,
           cherokee_virtual_server_t *vsrv)
{
	ret_t              ret;
	cherokee_buffer_t *tmp = NULL;

	UNUSED(vsrv);

	ret = cherokee_config_node_read (conf, "countries", &tmp);
	if (ret != ret_ok) {
		LOG_CRITICAL (CHEROKEE_ERROR_RULE_NO_PROPERTY,
			      RULE(rule)->priority, "geoip");
		return ret_error;
	}

	return parse_contry_list (tmp, &rule->countries);
}

static ret_t
_free (void *p)
{
	cherokee_rule_geoip_t *rule = RULE_GEOIP(p);

	if (rule->geoip) {
		geoip_release();
	}

	return ret_ok;
}

ret_t
cherokee_rule_geoip_new (cherokee_rule_t **rule)
{
	CHEROKEE_NEW_STRUCT (n, rule_geoip);

	/* Parent class constructor
	 */
	cherokee_rule_init_base (RULE(n), PLUGIN_INFO_PTR(geoip));

	/* Virtual methods
	 */
	RULE(n)->match     = (rule_func_match_t) match;
	RULE(n)->configure = (rule_func_configure_t) configure;
	MODULE(n)->free    = (module_func_free_t) _free;

	/* Properties
	 */
	n->geoip = geoip_get();
	if (n->geoip == NULL)
		return ret_error;

	cherokee_avl_init (&n->countries);

	*rule = RULE(n);
	return ret_ok;
}

