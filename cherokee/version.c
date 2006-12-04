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
#include "version.h"


ret_t 
cherokee_version_add (cherokee_buffer_t *buf, cherokee_server_token_t level)
{
	ret_t ret;

	switch (level) {
	case cherokee_version_product:
		ret = cherokee_buffer_add_str (buf, "Cherokee web server");
	case cherokee_version_minor:
		ret = cherokee_buffer_add_str (buf, "Cherokee web server " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION);
	case cherokee_version_minimal:
		ret = cherokee_buffer_add_str (buf, "Cherokee web server " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION);
	case cherokee_version_os:
		ret = cherokee_buffer_add_str (buf, "Cherokee web server " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION " (" OS_TYPE ")");
	case cherokee_version_full:
		ret = cherokee_buffer_add_str (buf, "Cherokee web server " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION " (" OS_TYPE ")");
	}

	return ret;
}


ret_t 
cherokee_version_add_w_port (cherokee_buffer_t *buf, cherokee_server_token_t level, cuint_t port)
{
	ret_t ret;

	switch (level) {
	case cherokee_version_product:
		ret = cherokee_buffer_add_va (buf, "Cherokee web server Port %d", port);
	case cherokee_version_minor:
		ret = cherokee_buffer_add_va (buf, "Cherokee web server Port %d " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION, port);
	case cherokee_version_minimal:
		ret = cherokee_buffer_add_va (buf, "Cherokee web server Port %d " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION, port);
	case cherokee_version_os:
		ret = cherokee_buffer_add_va (buf, "Cherokee web server Port %d " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION " (" OS_TYPE ")", port);
	case cherokee_version_full:
		ret = cherokee_buffer_add_va (buf, "Cherokee web server Port %d " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION " (" OS_TYPE ")", port);
	}

	return ret;
}
