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
#include "module.h"

ret_t
cherokee_module_init_base (cherokee_module_t       *module,
                           cherokee_module_props_t *props,
                           cherokee_plugin_info_t  *info)
{
	/* Properties
	 */
	module->info     = info;
	module->props    = props;

	/* Virtual methods
	 */
	module->instance = NULL;
	module->init     = NULL;
	module->free     = NULL;

	return ret_ok;
}


ret_t
cherokee_module_get_name (cherokee_module_t *module, const char **name)
{
	if (module->info == NULL)
		return ret_not_found;

	if (module->info->name == NULL) {
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	*name = module->info->name;
	return ret_ok;
}



/* Module properties
 */
ret_t
cherokee_module_props_init_base (cherokee_module_props_t *prop, module_func_props_free_t free_func)
{
	prop->free = free_func;
	return ret_ok;
}


ret_t
cherokee_module_props_free (cherokee_module_props_t *prop)
{
	if (prop == NULL)
		return ret_error;

	if (prop->free == NULL) {
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	prop->free (prop);
	return ret_ok;
}


ret_t
cherokee_module_props_free_base (cherokee_module_props_t *prop)
{
	free (prop);
	return ret_ok;
}
