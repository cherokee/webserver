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

#ifndef CHEROKEE_HEADER_H
#define CHEROKEE_HEADER_H

#include "common-internal.h"

#include <sys/types.h>

#include "header.h"


typedef struct {
	off_t info_off;
	off_t info_len;
} cherokee_header_entry_t;

typedef struct {
	off_t header_off;
	off_t header_info_off;
	int   header_info_len;
} cherokee_header_unknown_entry_t;


struct cherokee_header {
	/* Known headers
	 */
	cherokee_header_entry_t header[HEADER_LENGTH];

	/* Unknown headers
	 */
	cherokee_header_unknown_entry_t *unknowns;
	cint_t                           unknowns_len;

	/* Properties
	 */
	cherokee_header_type_t  type;
	cherokee_http_version_t version;
	cherokee_http_method_t  method;
	cherokee_http_t         response;

	/* Request
	 */
	off_t                   request_off;
	cint_t                  request_len;
	cint_t                  request_args_len;

	/* Query string
	 */
	off_t                   query_string_off;
	cint_t                  query_string_len;

	/* Debug & sanity checks
	 */
	cherokee_buffer_t      *input_buffer;
	crc_t                   input_buffer_crc;
	uint32_t                input_header_len;
};


#define HDR_METHOD(h)   (HDR(h)->method)
#define HDR_VERSION(h)  (HDR(h)->version)
#define HDR_RESPONSE(h) (HDR(h)->response)


#endif /* CHEROKEE_HEADER_H  */
