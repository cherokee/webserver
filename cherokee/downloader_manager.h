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

#ifndef CHEROKEE_DOWNLOADER_MGR_H
#define CHEROKEE_DOWNLOADER_MGR_H

#include <cherokee/common.h>
#include <cherokee/list.h>
#include <cherokee/downloader.h>
#include <cherokee/config_node.h>

CHEROKEE_BEGIN_DECLS

typedef struct {
	CHEROKEE_MUTEX_T (mutex);

	cherokee_list_t    available;
	cherokee_list_t    assigned;

	/* Limits */
	cuint_t            min;
	cuint_t            max;
	cuint_t            smax;

	/* Properties */
	cherokee_boolean_t keepalive;
} cherokee_downloader_mgr_t;

ret_t cherokee_downloader_mgr_init      (cherokee_downloader_mgr_t *mgr);
ret_t cherokee_downloader_mgr_mrproper  (cherokee_downloader_mgr_t *mgr);

ret_t cherokee_downloader_mgr_configure (cherokee_downloader_mgr_t *mgr,
					 cherokee_config_node_t    *conf);

ret_t cherokee_downloader_mgr_get      (cherokee_downloader_mgr_t *mgr,
					cherokee_downloader_t    **down);
ret_t cherokee_downloader_mgr_release  (cherokee_downloader_mgr_t *mgr,
					cherokee_downloader_t     *down);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_DOWNLOADER_MGR_H */
