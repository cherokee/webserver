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

#if !defined (CHEROKEE_INSIDE_CHEROKEE_H) && !defined (CHEROKEE_COMPILATION)
# error "Only <cherokee/cherokee.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef CHEROKEE_ASYNC_DOWNLOADER__H
#define CHEROKEE_ASYNC_DOWNLOADER_H

#include <cherokee/common.h>
#include <cherokee/downloader.h>

CHEROKEE_BEGIN_DECLS

typedef struct cherokee_downloader_async cherokee_downloader_async_t;
#define DOWNLOADER_ASYNC(d) ((cherokee_downloader_async_t *)(d))

ret_t cherokee_downloader_async_new        (cherokee_downloader_async_t **downloader);
ret_t cherokee_downloader_async_free       (cherokee_downloader_async_t  *downloader);
ret_t cherokee_downloader_async_init       (cherokee_downloader_async_t  *downloader);
ret_t cherokee_downloader_async_mrproper   (cherokee_downloader_async_t  *downloader);

ret_t cherokee_downloader_async_set_fdpoll (cherokee_downloader_async_t  *downloader, cherokee_fdpoll_t *fdpoll);
ret_t cherokee_downloader_async_connect    (cherokee_downloader_async_t  *downloader);
ret_t cherokee_downloader_async_step       (cherokee_downloader_async_t  *downloader);

CHEROKEE_END_DECLS

#endif /* CHEROKEE_ASYNC_DOWNLOADER_H */
