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

#ifndef CHEROKEE_EXTS_TABLE_H
#define CHEROKEE_EXTS_TABLE_H

#include "common.h"
#include "buffer.h"
#include "config_entry.h"

typedef struct cherokee_exts_table cherokee_exts_table_t;      /* Extension -> config_entry */
#define EXTTABLE(x) ((cherokee_exts_table_t *)(x))

ret_t cherokee_exts_table_new  (cherokee_exts_table_t **et);
ret_t cherokee_exts_table_free (cherokee_exts_table_t  *et);

ret_t cherokee_exts_table_get  (cherokee_exts_table_t *et, cherokee_buffer_t *requested_url, cherokee_config_entry_t *plugin_entry);
ret_t cherokee_exts_table_add  (cherokee_exts_table_t *et, char *ext, cherokee_config_entry_t  *plugin_entry);
ret_t cherokee_exts_table_has  (cherokee_exts_table_t *et, char *ext);

#endif /* CHEROKEE_EXTS_TABLE_H */
