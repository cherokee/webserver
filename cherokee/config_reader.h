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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_CONFIG_READER_H
#define CHEROKEE_CONFIG_READER_H

#include <cherokee/common.h>
#include <cherokee/config_node.h>

CHEROKEE_BEGIN_DECLS

ret_t cherokee_config_reader_parse        (cherokee_config_node_t *conf, cherokee_buffer_t *path);
ret_t cherokee_config_reader_parse_string (cherokee_config_node_t *conf, cherokee_buffer_t *string);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_CONFIG_READER_H */
