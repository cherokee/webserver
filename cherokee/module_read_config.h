
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

#ifndef CHEROKEE_MODULE_READ_CONFIG_H
#define CHEROKEE_MODULE_READ_CONFIG_H

#include "common-internal.h"
#include "module.h"
#include "server.h"

typedef struct {
	   cherokee_module_t base;
} cherokee_module_read_config_t;


ret_t cherokee_module_read_config_new  (cherokee_module_read_config_t **config);
ret_t cherokee_module_read_config_free (cherokee_module_read_config_t  *config);

ret_t read_config_file   (cherokee_server_t *srv, char *filename);
ret_t read_config_string (cherokee_server_t *srv, char *config_string);

#endif /* CHEROKEE_MODULE_READ_CONFIG_H */
