/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *      Christopher Pruden <pruden@dyndns.org>
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


#include "common-internal.h"
#include "header.h"
#include "header-protected.h"

#include "util.h"

#ifdef HAVE_STRING_H
# include <string.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <ctype.h>


// #define HEADER_INTERNAL_DEBUG

#ifdef HEADER_INTERNAL_DEBUG
# define HEADER_INTERNAL_CHECK(h)       	                                                   \
	do { if (cherokee_buffer_crc32 (h->input_buffer) != h->input_buffer_crc)                   \
		fprintf (stderr, "Header sanity check failed: %s, line %d\n", __FILE__, __LINE__); \
        } while(0)
#else
# define HEADER_INTERNAL_CHECK(h)
#endif

#define cmp_str(l,s) (strncmp(l, s, sizeof(s)-1) == 0)


static void
clean_known_headers (cherokee_header_t *hdr)
{
	int i;
	
	for (i=0; i<HEADER_LENGTH; i++) {
		hdr->header[i].info_off =  0;
		hdr->header[i].info_len = -1;			 
	}
}

static void
clean_unknown_headers (cherokee_header_t *hdr)
{
	if (hdr->unknowns != NULL) {
		free (hdr->unknowns);
		hdr->unknowns     = NULL;
		hdr->unknowns_len = 0;
	}
}

static void
clean_headers (cherokee_header_t *hdr)
{
	clean_known_headers (hdr);
	clean_unknown_headers (hdr);
}


ret_t 
cherokee_header_init (cherokee_header_t *hdr, cherokee_header_type_t type)
{
	/* Known headers
	 */
	hdr->type = type;
	clean_known_headers (hdr);

	/* Unknown headers
	 */
	hdr->unknowns     = NULL;
	hdr->unknowns_len = 0;

	/* Properties
	 */
	hdr->method   = http_unknown;
	hdr->version  = http_version_unknown;
	hdr->response = http_unset; 

	/* Request
	 */
	hdr->request_off      = 0;
	hdr->request_len      = 0;
	hdr->request_args_len = 0;
	
	/* Query string
	 */
	hdr->query_string_off = 0;
	hdr->query_string_len = 0;

	/* Sanity
	 */
	hdr->input_buffer       = NULL;
	hdr->input_buffer_crc   = 0;
	hdr->input_header_len   = 0;

	return ret_ok;
}

ret_t 
cherokee_header_mrproper (cherokee_header_t *hdr)
{
	clean_unknown_headers (hdr);
	return ret_ok;
}

ret_t 
cherokee_header_new (cherokee_header_t **hdr, cherokee_header_type_t type)
{
	ret_t ret;
	CHEROKEE_NEW_STRUCT (n, header);

	ret = cherokee_header_init (n, type);
	if (unlikely (ret != ret_ok)) return ret;

	*hdr = n;
	return ret_ok;
}

CHEROKEE_ADD_FUNC_FREE (header);


ret_t 
cherokee_header_clean (cherokee_header_t *hdr)
{
	clean_headers (hdr);	

	hdr->method   = http_unknown;
	hdr->version  = http_version_unknown;
	hdr->response = http_unset; 

	hdr->input_header_len = 0;

	hdr->request_off = 0;
	hdr->request_len = 0;

	hdr->query_string_off = 0;
	hdr->query_string_len = 0;	
	
	return ret_ok;
}


static ret_t 
add_known_header (cherokee_header_t *hdr, cherokee_common_header_t header, off_t info_off, int info_len)
{
	HEADER_INTERNAL_CHECK(hdr);

	hdr->header[header].info_off = info_off;
	hdr->header[header].info_len = info_len;

	return ret_ok;
}


static ret_t 
add_unknown_header (cherokee_header_t *hdr, off_t header_off, off_t info_off, int info_len)
{
	cherokee_header_unknown_entry_t *entry;

	hdr->unknowns_len++;

	hdr->unknowns = (cherokee_header_unknown_entry_t *) realloc (
		hdr->unknowns, sizeof(cherokee_header_unknown_entry_t) * hdr->unknowns_len);
				 
	if (hdr->unknowns == NULL) {
		return ret_nomem;
	}
			
	entry = &hdr->unknowns[hdr->unknowns_len-1];
	entry->header_off      = header_off;
	entry->header_info_off = info_off;
	entry->header_info_len = info_len;
	
	return ret_ok;
}


static ret_t
parse_response_first_line (cherokee_header_t *hdr, cherokee_buffer_t *buf, char **next_pos, cherokee_http_t *error_code)
{
	char *line  = buf->buf;
	char *begin = buf->buf;
	char  tmp[4];
	char *end;
	size_t  len;

	/* NOTE: here we deal only with HTTP/1.0 or higher.
	 */
	len = strcspn(line, CRLF);
	end = &line[len];

	/* Some security checks
	 */
	if (len < 14 || buf->len < 14) {
		return ret_error;
	}

	/* Return next line
	 */
	switch (*end) {
		case CHR_CR:
			if (end[1] != CHR_LF)
				return ret_error;
			*next_pos = end + 2;
			break;
		case CHR_LF:
			*next_pos = end + 1;
			break;
		default:
			return ret_error;
	}

	/* Example:
	 * HTTP/1.0 403 Forbidden
	 */
	if (unlikely(! cmp_str(begin, "HTTP/1."))) {
		/* TODO: improve parser
		 * (if ! "HTTP/" then leave default http_bad_request).
		 */
		*error_code = http_version_not_supported;
		return ret_error;
	}

	if (unlikely(! isdigit(begin[7]))) {
		return ret_error;
	}

	/* Get the HTTP version
	 */
	switch (begin[7]) {
	case '0':
		hdr->version = http_version_10; 
		break;
	case '1':
	default:
		hdr->version = http_version_11; 
		break;
	}

	/* Read the response code
	 * TODO: skip spaces properly (usually there is only one blank).
	 */
	memcpy (tmp, begin+9, 3);
	tmp[3] = '\0';
	hdr->response = (cherokee_http_t) atoi (tmp);

	return ret_ok;
}


static ret_t
parse_method (cherokee_header_t *hdr, char *line, char **pointer)
{
	char chr = *line;

#define detect_method(l,str,mthd)           		  \
	if (cmp_str (line, (str" "))) { \
		hdr->method = http_ ## mthd;    \
		*pointer += sizeof(str);        \
		return ret_ok;                  \
	}

	/* Check the first letter of the method name, if it matches it
	 * can continue with the rest.
	 */
	switch (chr) {
	case 'G':
		detect_method (line, "GET", get)
		break;
	case 'P':
		detect_method (line, "POST", post)
		else
		detect_method (line, "PUT", put)
		else
		detect_method (line, "POLL", poll)
		else
		detect_method (line, "PROPFIND", propfind)
		else
		detect_method (line, "PROPPATCH", proppatch)
		break;
	case 'H':
		detect_method (line, "HEAD", head)
		break;
	case 'O':
		detect_method (line, "OPTIONS", options)
		break;
	case 'D':
		detect_method (line, "DELETE", delete)
		break;
	case 'T':
		detect_method (line, "TRACE", trace)
		break;
	case 'C':
		detect_method (line, "CONNECT", connect)
		else
		detect_method (line, "COPY", copy)
		break;
	case 'L':
		detect_method (line, "LOCK", lock)
	        break;
	case 'M':
		detect_method (line, "MKCOL", mkcol)
		else 
		detect_method (line, "MOVE", move)
	        break;
	case 'N':
		detect_method (line, "NOTIFY", notify)
	        break;
	case 'S':
		detect_method (line, "SEARCH", search)
		else
		detect_method (line, "SUBSCRIBE", subscribe)
	        break;
	case 'U':
		detect_method (line, "UNLOCK", unlock)
		else
		detect_method (line, "UNSUBSCRIBE", unsubscribe)
	        break;
	}

	return ret_error;
}


static ret_t
parse_request_first_line (cherokee_header_t *hdr, cherokee_buffer_t *buf, char **next_pos, cherokee_http_t *error_code)
{
	ret_t  ret;
	char  *line  = buf->buf;
	char  *begin = buf->buf;
	char  *begin_ver = buf->buf;
	char  *end;
	char  *ptr;
	char  *restore;
	char  chr_end;
	size_t len = 0;
	size_t len_ver = 0;

	/* Basic security check. The shortest possible request
	 * "GET / HTTP/1.0" is 14 characters long but we want
	 * to reply (with an error message) to HTTP/0.9 requests too
	 * (HTTP/0.9 don't have " HTTP/x.x" version).
	 */
	if (unlikely(buf->len < 6)) {
		return ret_error;
	}

	/* Look for the end of the request line
	 */
	end = strchr (line, CHR_LF);
	if (unlikely(end == NULL || line == end)) {
		return ret_error;
	}
	--end;
	if (unlikely(*end != CHR_CR)) {
		++end;
		/* Return begin of next line 
		 */
		len = (size_t) (end - line);
		*next_pos = end + 1;
	} else {
		/* Return begin of next line 
		 */
		len = (size_t) (end - line);
		*next_pos = end + 2;
	}
	chr_end = *end;
	*end = '\0';
	restore = end;

	/* Check shortest string length "GET /"
	 */
	if (len < 5)
		goto error;

	/* Get the method
	 */
	ret = parse_method (hdr, line, &begin);
	if (unlikely (ret != ret_ok)) {
		*error_code = http_not_implemented;
		goto error;
	}

	/* Skip other blank spaces between method and URI.
	 */
	begin += strspn (begin, LBS);

	/* Length of URI.
	 */
	len = strcspn (begin, LBS);
	if (len == 0)
		goto error;

	if (begin[len] == '\0') {
		/* HTTP/0.9 has no HTTP version string and
		 * it is not supported by this server.
		 */
		*error_code = http_version_not_supported;
		goto error;
	}

	begin_ver = &begin[len];
	begin_ver += strspn (begin_ver, LBS);
	len_ver = (size_t) (end - begin_ver);

	/* Verify "HTTP/x.x" pattern.
	 */
	if (len_ver != 8 ||
	    !isdigit(begin_ver[5]) ||
	    begin_ver[6] != '.' ||
	    !isdigit(begin_ver[7]) ||
	    !cmp_str (begin_ver, "HTTP/")) {
		goto error;
	}

	/* Get the protocol version.
	 */
	if (begin_ver[5] != '1' || begin_ver[7] > '1') {
		*error_code = http_version_not_supported;
		goto error;
	}

	hdr->version = (begin_ver[7] == '1' ? http_version_11 : http_version_10); 

	/* Skip the HTTP version string: " HTTP/x.y"
	 */
	end = &begin[len];

	/* Look for the QueryString
	 */
	hdr->request_args_len = len;
	ptr = strchr (begin, '?');

	if (ptr) {
		end = ptr;
		hdr->query_string_off = (off_t) (++ptr - buf->buf);
		hdr->query_string_len = (cint_t) (&begin[len] - ptr);
	} else {
		hdr->query_string_len = 0;
	}

	/* Get the request
	 */
	hdr->request_off = (off_t)  (begin - buf->buf);
	hdr->request_len = (cint_t) (end - begin);

	/* Check if the request is a full URL
	 */
	begin = buf->buf + hdr->request_off;
	if (cmp_str (begin, "http://")) {
		char   *dir;
		char   *host = begin + 7;
		char   end_chr = *end;

		if (host[0] == '/' || host[0] == '.')
			goto error;

		*end = '\0';
		dir = strchr (host, '/');
		*end = end_chr;
		if (dir == NULL)
			goto error;

		/* Add the host header
		 */
		add_known_header (hdr, header_host, host - buf->buf, dir - host);
		
		/* Fix the URL
		 */
		hdr->request_len -= (dir - begin);
		hdr->request_off = dir - buf->buf;
	}

	*restore = chr_end;
	return ret_ok;

error:
	*restore = chr_end;
	return ret_error;
}


static char *
get_new_line (char *string)
{
	char *end1;
	char *end2 = string;

	/* RFC states that EOL should be made by CRLF only, but some
	 * old clients (HTTP 0.9 and a few HTTP/1.0 robots) may send
	 * LF only as EOL, so try to catch that case too (of course CR
	 * only is ignored); anyway leading spaces after a LF or CRLF
	 * means that the new line is the continuation of previous
	 * line (first line excluded).
	 */
	do {
		end1 = end2;
		end2 = strchr (end1, CHR_LF);

		if (unlikely (end2 == NULL))
			return NULL;

		end1 = end2;
		if (likely (end2 != string && *(end1 - 1) == CHR_CR))
			--end1;

		++end2;
	} while (*end2 == CHR_SP || *end2 == CHR_HT);

	return end1;
}


ret_t 
cherokee_header_get_length (cherokee_header_t *hdr, cuint_t *len)
{
	*len = hdr->input_header_len;
	return ret_ok;
}


ret_t 
cherokee_header_get_unknown (cherokee_header_t *hdr, char *name, int name_len, char **header, cuint_t *header_len)
{
	int i;

	HEADER_INTERNAL_CHECK(hdr);

	for (i=0; i < hdr->unknowns_len; i++) {
		char *h = hdr->unknowns[i].header_off + hdr->input_buffer->buf;

		if (strncasecmp (h, name, name_len) == 0) {
			*header     = hdr->unknowns[i].header_info_off + hdr->input_buffer->buf;
			*header_len = hdr->unknowns[i].header_info_len;

			return ret_ok;
		}
	}

	return ret_not_found;
}


ret_t
cherokee_header_copy_unknown (cherokee_header_t *hdr, char *name, int name_len, cherokee_buffer_t *buf)
{
	ret_t    ret;
	char    *info;
	cuint_t  info_len;

	ret = cherokee_header_get_unknown (hdr, name, name_len, &info, &info_len);
	if (unlikely(ret != ret_ok))
		return ret;

	return cherokee_buffer_add (buf, info, info_len);
}


ret_t 
cherokee_header_has_known (cherokee_header_t *hdr, cherokee_common_header_t header)
{
	if (hdr->header[header].info_off != 0) {
		return ret_ok;
	}

	return ret_not_found;
}


ret_t 
cherokee_header_get_known (cherokee_header_t *hdr, cherokee_common_header_t header, char **info, cuint_t *info_len)
{
	HEADER_INTERNAL_CHECK(hdr);

	if (hdr->header[header].info_off == 0) {
		return ret_not_found;
	}

	*info     = hdr->header[header].info_off + hdr->input_buffer->buf;
	*info_len = hdr->header[header].info_len;

	return ret_ok;
}


ret_t 
cherokee_header_copy_known (cherokee_header_t *hdr, cherokee_common_header_t header, cherokee_buffer_t *buf)
{
	ret_t    ret;
	char    *info     = NULL;
	cuint_t  info_len = 0;

	ret = cherokee_header_get_known (hdr, header, &info, &info_len);
	if (unlikely(ret != ret_ok))
		return ret;

	return cherokee_buffer_add (buf, info, info_len); 
}


ret_t 
cherokee_header_copy_request (cherokee_header_t *hdr, cherokee_buffer_t *request)
{
	ret_t ret;

	if ((hdr->request_off == 0) ||
	    (hdr->request_len <= 0)) {
		return ret_error;
	}

	ret = cherokee_buffer_add (request, hdr->input_buffer->buf + hdr->request_off, hdr->request_len);
	if (unlikely(ret < ret_ok))
		return ret;

	return cherokee_buffer_unescape_uri (request);
}


ret_t 
cherokee_header_copy_query_string (cherokee_header_t *hdr, cherokee_buffer_t *query_string)
{
	ret_t ret;

	if ((hdr->query_string_off == 0) ||
	    (hdr->query_string_len <= 0)) {
		return ret_not_found;
	}

	ret = cherokee_buffer_add (query_string, hdr->input_buffer->buf + hdr->query_string_off, hdr->query_string_len);
	if (unlikely(ret < ret_ok))
		return ret;

	return ret_ok;
}


ret_t 
cherokee_header_get_request_w_args (cherokee_header_t *hdr, char **req, int *req_len)
{
	if ((hdr->request_off == 0) ||
	    (hdr->request_args_len <= 0)) {
		return ret_error;
	}

	if (req != NULL)
		*req = hdr->input_buffer->buf + hdr->request_off;

	if (req_len != NULL)
		*req_len = hdr->request_args_len;
	
	return ret_ok;
}


ret_t
cherokee_header_copy_request_w_args (cherokee_header_t *hdr, cherokee_buffer_t *request)
{
	ret_t ret;

	if ((hdr->request_off == 0) ||
	    (hdr->request_args_len <= 0)) {
		return ret_error;
	}

	ret = cherokee_buffer_add (request, hdr->input_buffer->buf + hdr->request_off, hdr->request_args_len);
	if (unlikely(ret < ret_ok))
		return ret;

	return cherokee_buffer_unescape_uri (request);
}


ret_t 
cherokee_header_copy_method (cherokee_header_t *hdr, cherokee_buffer_t *buf)
{
	ret_t       ret;
	const char *tmp;
	cuint_t     len;

	ret = cherokee_http_method_to_string (HDR_METHOD(hdr), &tmp, &len);
	if (unlikely(ret != ret_ok))
		return ret;

	return cherokee_buffer_add (buf, tmp, len);
}


ret_t 
cherokee_header_copy_version (cherokee_header_t *hdr, cherokee_buffer_t *buf)
{
	ret_t       ret;
	const char *tmp;
	cuint_t     len;

	ret = cherokee_http_version_to_string (HDR_VERSION(hdr), &tmp, &len);
	if (unlikely(ret != ret_ok))
		return ret;

	return cherokee_buffer_add (buf, tmp, len);
}


static ret_t 
has_header_response (cherokee_header_t *hdr, cherokee_buffer_t *buffer)
{
	char *tmp;

	tmp = strstr (buffer->buf, CRLF_CRLF);
	if (tmp == NULL) 
		return ret_not_found;

	/* Content until CRLF_CRLF plus two of the last line CRLF
	 */
	hdr->input_header_len = (tmp - buffer->buf) + 2;
	return ret_ok;
}


static ret_t 
has_header_request (cherokee_header_t *hdr, cherokee_buffer_t *buffer, cuint_t tail_len)
{
	char   *start;
	char   *end;
	size_t  crlf_len = 0;

	/* Skip initial CRLFs:
	 * NOTE: they are not allowed by standard (RFC) but many
	 * popular HTTP clients (including MSIE, etc.)  may send a
	 * couple of them between two requests, so every widely used
	 * web server has to deal with them.
	 */
	crlf_len = cherokee_buffer_cnt_spn (buffer, 0, CRLF);
	if (unlikely (crlf_len > MAX_HEADER_CRLF)) {
		/* Too many initial CRLF 
		 */
		return ret_error;
	}

	if ((crlf_len > 0) && (crlf_len < (size_t) buffer->len)) {
		/* Found heading CRLFs and their length is less than
		 * buffer length so we have to move the real content
		 * to the beginning of the buffer.
		 */
		cherokee_buffer_move_to_begin(buffer, (int) crlf_len);
	}

	/* Do we have enough information ?
	 * len("GET /" LF_LF) = 7                   (HTTP/0.9)
	 * len("GET /" CRLF_CRLF) = 9               (HTTP/0.9)
	 * len("GET / HTTP/1.0" CRLF_CRLF) = 18     (HTTP/1.x)
	 */
	if (unlikely (buffer->len < 7)) {
		return ret_not_found;
	}

	/* Look for the starting point
	 */
	start = (tail_len >= buffer->len) ?
		buffer->buf : buffer->buf + (buffer->len - tail_len);

	/* It could be a partial header, or maybe a POST request
	 */
	if (unlikely ((end = strstr(start, CRLF_CRLF)) == NULL)) {
		if ((end = strstr (start, LF_LF)) == NULL)
			return ret_not_found;

		/* Found uncommon / non standard EOH, set header length 
		 */
		hdr->input_header_len = ((int) (end - buffer->buf)) + CSZLEN(LF_LF);
		return ret_ok;
	}

	/* Found standard EOH, set header length 
	 */
	hdr->input_header_len = ((int) (end - buffer->buf)) + CSZLEN(CRLF_CRLF);

	return ret_ok;
}


ret_t 
cherokee_header_has_header (cherokee_header_t *hdr, cherokee_buffer_t *buffer, int tail_len)
{
	switch (hdr->type) {
	case header_type_request:
		return has_header_request (hdr, buffer, tail_len);

	case header_type_response:
	case header_type_basic:
		return has_header_response (hdr, buffer);

	default:
		SHOULDNT_HAPPEN;
	}

	return ret_error;
}


ret_t 
cherokee_header_parse (cherokee_header_t *hdr, cherokee_buffer_t *buffer, cherokee_http_t *error_code)
{
	ret_t  ret;
	char  *val_beg;
	char  *val_end;
	char  *header_end;
	char  chr_header_end;
	char  *begin           = buffer->buf;
	char  *end             = NULL;

	/* Set default error code.
	 */
	*error_code = http_bad_request;

	/* Check the buffer content
	 */
	if ((buffer->buf == NULL) || (buffer->len < 5)) {
		PRINT_ERROR_S ("ERROR: Calling cherokee_header_parse() with an empty header\n");
		return ret_error;
	}

	/* Set the header buffer
	 */
	hdr->input_buffer = buffer;

#ifdef HEADER_INTERNAL_DEBUG
	hdr->input_buffer_crc = cherokee_buffer_crc32 (buffer);
#endif

	/* header_len should have already been set by a previous call
	 * to cherokee_header_has_header() but if not we call it now.
	 */
	if (unlikely (hdr->input_header_len < 1)) {
		/* Strange, anyway go on and look for EOH 
		 */
		ret = has_header_request (hdr, buffer, buffer->len);
		if (ret != ret_ok) {
			if (ret == ret_not_found)
				PRINT_ERROR("ERROR: EOH not found:\n===\n%s===\n", buffer->buf);
			else
				PRINT_ERROR("ERROR: Too many initial CRLF:\n===\n%s===\n", buffer->buf);
			return ret_error;
		}
	}
	header_end = &(buffer->buf[hdr->input_header_len]);

	/* Terminate current request space (there may be other
	 * pipelined requests in the buffer) after the EOH.
	 */
	chr_header_end = *header_end;
	*header_end = '\0';

	/* Parse the special first line
	 */
	switch (hdr->type) {
	case header_type_request:
		/* Parse request. Something like this:
		 * GET /icons/compressed.png HTTP/1.1CRLF
		 */
		ret = parse_request_first_line (hdr, buffer, &begin, error_code);
		if (unlikely(ret < ret_ok)) {
			PRINT_DEBUG ("ERROR: Failed to parse header_type_request:\n===\n%s===\n", buffer->buf);
			*header_end = chr_header_end;
			return ret;
		}
		break;

	case header_type_response:
		ret = parse_response_first_line (hdr, buffer, &begin, error_code);
		if (unlikely(ret < ret_ok)) {
			PRINT_DEBUG ("ERROR: Failed to parse header_type_response:\n===\n%s===\n", buffer->buf);
			*header_end = chr_header_end;
			return ret;
		}
		break;

	case header_type_basic:
		/* Don't do anything
		 */
		break;

	default:
		*header_end = chr_header_end;
		SHOULDNT_HAPPEN;
	}


	/* Parse the rest of headers
	 */
	while ((begin < header_end)) {
		cuint_t header_len;
		int	val_offs;
		int	val_len;
		char    first_char;
		char    chr_end;

		/* Check where the line ends
		 */
		end = get_new_line(begin);
		if (end == NULL)
			break;

		chr_end = *end;;
		*end    = '\0';

		/* Current line may have embedded CR+SP or CRLF+SP 
		 */
		val_beg = strchr (begin, ':');
		if (unlikely (val_beg == NULL))
			goto next;

		/* Header name must be at least 1 character long.
		 */
		if (unlikely (val_beg == begin))
			goto next;

		/* Empty values are skipped.
		 */
		if (unlikely (end <= val_beg + 1))
			goto next;

		header_len = val_beg - begin;

		/* Trim heading and leading spaces from header value.
		 */
		++val_beg;
		val_beg += strspn(val_beg, LWS);
		val_end = end - 1;
		while (val_end > val_beg && isspace(*val_end)) {
			val_end--;
		}
		val_end++;

		val_offs = val_beg - buffer->buf;
		val_len  = val_end - val_beg;
		if (unlikely (val_len < 1))
			goto next;

		first_char = *begin;
		if (first_char > 'Z')
			first_char -=  'a' - 'A';


#define header_equals(str,hdr_enum,begin,len) ((len == (sizeof(str)-1)) && \
					       (hdr->header[hdr_enum].info_off == 0) && \
					       (strncasecmp (begin, str, sizeof(str)-1) == 0))

		switch (first_char) {
		case 'A':
			if (header_equals ("Accept-Encoding", header_accept_encoding, begin, header_len)) {
				ret = add_known_header (hdr, header_accept_encoding, val_offs, val_len);
			} else if (header_equals ("Accept-Charset", header_accept_charset, begin, header_len)) {
				ret = add_known_header (hdr, header_accept_charset, val_offs, val_len);
			} else if (header_equals ("Accept-Language", header_accept_language, begin, header_len)) {
				ret = add_known_header (hdr, header_accept_language, val_offs, val_len);
			} else if (header_equals ("Accept", header_accept, begin, header_len)) {
				ret = add_known_header (hdr, header_accept, val_offs, val_len);
			} else if (header_equals ("Authorization", header_authorization, begin, header_len)) {
				ret = add_known_header (hdr, header_authorization, val_offs, val_len);
			} else
				goto unknown; 
			break;
		case 'C':
			if (header_equals ("Connection", header_connection, begin, header_len)) {
				ret = add_known_header (hdr, header_connection, val_offs, val_len);
			} else if (header_equals ("Content-Length", header_content_length, begin, header_len)) {
				ret = add_known_header (hdr, header_content_length, val_offs, val_len);
			} else if (header_equals ("Cookie", header_cookie, begin, header_len)) {
				ret = add_known_header (hdr, header_cookie, val_offs, val_len);
			} else
				goto unknown;
			break;
		case 'H':
			if (header_equals ("Host", header_host, begin, header_len)) {
				ret = add_known_header (hdr, header_host, val_offs, val_len);
			} else
				goto unknown;
			break;
		case 'I':
			if (header_equals ("If-Modified-Since", header_if_modified_since, begin, header_len)) {
				ret = add_known_header (hdr, header_if_modified_since, val_offs, val_len);
			} else if (header_equals ("If-None-Match", header_if_none_match, begin, header_len)) {
				ret = add_known_header (hdr, header_if_none_match, val_offs, val_len);
			} else if (header_equals ("If-Range", header_if_range, begin, header_len)) {
				ret = add_known_header (hdr, header_if_range, val_offs, val_len);
			} else
				goto unknown;
			break;
		case 'K':
			if (header_equals ("Keep-Alive", header_keepalive, begin, header_len)) {
				ret = add_known_header (hdr, header_keepalive, val_offs, val_len);
			} else
				goto unknown;
			break;
		case 'L':
			if (header_equals ("Location", header_location, begin, header_len)) {
				ret = add_known_header (hdr, header_location, val_offs, val_len);
			} else
				goto unknown;
			break;
		case 'R':
			if (header_equals ("Range", header_range, begin, header_len)) {
				ret = add_known_header (hdr, header_range, val_offs, val_len);
			} else if (header_equals ("Referer", header_referer, begin, header_len)) {
				ret = add_known_header (hdr, header_referer, val_offs, val_len);
			} else
				goto unknown;
			break;
		case 'U':
			if (header_equals ("Upgrade", header_upgrade, begin, header_len)) {
				ret = add_known_header (hdr, header_upgrade, val_offs, val_len);
			} else if (header_equals ("User-Agent", header_user_agent, begin, header_len)) {
				ret = add_known_header (hdr, header_user_agent, val_offs, val_len);
			} else
				goto unknown;
			break;
		default:
		unknown:
			/* Add a unknown header
			 */
			ret = add_unknown_header (hdr, begin-buffer->buf, val_offs, val_len);
		}

		if (ret < ret_ok) {
			PRINT_ERROR_S ("ERROR: Failed to add_(un)known_header()\n");
			*header_end = chr_header_end;
			return ret;
		}

	next:
		*end = chr_end;

		while ((*end == CHR_CR) || (*end == CHR_LF))
			end++;
		begin = end;
	}

	*header_end = chr_header_end;
	return ret_ok;
}


ret_t 
cherokee_header_foreach_unknown (cherokee_header_t *hdr, cherokee_header_foreach_func_t func, void *data)
{
	int               i;
	cherokee_buffer_t tmp_hdr = CHEROKEE_BUF_INIT;
	cherokee_buffer_t tmp_val = CHEROKEE_BUF_INIT;
	
	HEADER_INTERNAL_CHECK(hdr);

	for (i=0; i < hdr->unknowns_len; i++) {
		char *begin      = hdr->unknowns[i].header_off      + hdr->input_buffer->buf;
		char *begin_info = hdr->unknowns[i].header_info_off + hdr->input_buffer->buf;

		cherokee_buffer_add (&tmp_hdr, begin, 
				     (hdr->unknowns[i].header_info_off - 2) - hdr->unknowns[i].header_off);
				     
		cherokee_buffer_add (&tmp_val, begin_info,
				     hdr->unknowns[i].header_info_len);

		func (&tmp_hdr, &tmp_val, data);

		cherokee_buffer_clean (&tmp_hdr);
		cherokee_buffer_clean (&tmp_val);
	}

	cherokee_buffer_mrproper (&tmp_hdr);
	cherokee_buffer_mrproper (&tmp_val);
	return ret_ok;
}
