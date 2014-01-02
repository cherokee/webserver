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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_TIME_H
# include <time.h>
#endif

#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#else
# include "getopt/getopt.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include "init.h"
#include "util.h"
#include "url.h"
#include "header.h"
#include "fdpoll.h"
#include "request.h"
#include "downloader.h"
#include "downloader-protected.h"
#include "socket.h"
#include "header-protected.h"      /* FIXME! */

#include "proxy.h"

#define EXIT_OK    0
#define EXIT_ERROR 1
#define POLL_TIME  1000
#define UNSET_FD   -1
#define COLUM_NUM  20
#define COLUM_SEP  ":"
#define ENTRIES    "cget"


/* Globals..
 */
static int                output_fd    = UNSET_FD;
static int                global_fd    = UNSET_FD;
static cherokee_boolean_t quiet        = false;
static cherokee_boolean_t save_headers = false;


static void
print_help (void)
{
	printf ("Cherokee Downloader %s\n"
	        "Usage: cget [options] URL\n\n"
	        "Mandatory arguments to long options are mandatory for short options too.\n\n"
	        "Startup:\n"
	        "  -V,  --version                Print version and exit\n"
	        "  -h,  --help                   Print this help\n"
	        "\n"
	        "Logging and input file:\n"
	        "  -q,  --quiet                  Quiet (no output)\n"
	        "\n"
	        "Download:\n"
	        "  -O   --output-document=FILE   Write documents to FILE\n"
	        "\n"
	        "HTTP options:\n"
	        "  -s,  --save-headers           Save the HTTP headers to file\n"
	        "       --header=STRING          insert STRING among the headers\n"
	        "\n"
	        "Report bugs to alvaro@alobbs.com\n", PACKAGE_VERSION);
}


static void
print_usage (void)
{
	printf ("Cherokee Downloader %s\n"
	        "Usage: cget [options] URL\n\n"
	        "Try `cget --help' for more options.\n", PACKAGE_VERSION);
}


static void
print_tuple_str (const char *title, const char *info)
{
	int   whites;

	if (quiet == true)
		return;

	whites = COLUM_NUM - strlen(title);

	fprintf (stderr, "%s ", title);
	while (whites-- > 0) {
		fprintf (stderr, " ");
	}
	fprintf (stderr, COLUM_SEP " %s\n", info);
}


static void
print_tuple_int (const char *title, int num)
{
	char tmp[128];

	snprintf (tmp, 128, "%d", num);
	print_tuple_str (title, tmp);
}


static ret_t
do_download__init (cherokee_downloader_t *downloader, void *param)
{
	cherokee_url_t *url;

	UNUSED(param);

	url = &downloader->request.url;

	print_tuple_str ("Host",    url->host.buf);
	print_tuple_str ("Request", url->request.buf);
	print_tuple_int ("Port",    url->port);

	return ret_ok;
}


static ret_t
do_download__has_headers (cherokee_downloader_t *downloader, void *param)
{
	cherokee_url_t    *url;
	cherokee_buffer_t *req;
	cherokee_header_t *hdr;

	UNUSED(param);

	url = &downloader->request.url;
	req = &url->request;
	hdr = downloader->header;

	TRACE(ENTRIES, "quiet=%d http_code=%d\n", quiet, hdr->response);

	/* Check the response
	 */
	if (quiet == false) {
		cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

		cherokee_http_code_copy (HDR_RESPONSE(hdr), &tmp);
		print_tuple_str ("Response", tmp.buf);
		cherokee_buffer_mrproper (&tmp);
	}

	/* Open a new file if needed
	 */
	if (global_fd == UNSET_FD) {
		char *name;

		name = strrchr (req->buf, '/');
		if (name == NULL) {
			name = "index.html";
		}

		output_fd = open (name, O_WRONLY, O_CREAT);
		if (output_fd < 0) {
			return ret_error;
		}

	} else {
		output_fd = global_fd;
	}

	/* Save the headers?
	 */
	if (save_headers == true) {
		ssize_t  written;
		uint32_t len;

		cherokee_header_get_length (hdr, &len);
		written = write (output_fd, hdr->input_buffer->buf, len);
		if (written < 0) {
			PRINT_MSG_S ("ERROR: Can not write to output file\n");
			return ret_error;
		}
	}

	return ret_ok;
}


static ret_t
do_download__read_body (cherokee_downloader_t *downloader, void *param)
{
 	ret_t             ret;
	ssize_t           len;
	cherokee_buffer_t tmp = CHEROKEE_BUF_INIT;

	UNUSED(param);

	/* Write down
	 */
	len = write (output_fd, downloader->body.buf, downloader->body.len);
	if (len > 0) {
		ret = cherokee_buffer_move_to_begin (&downloader->body, len);
		if (ret != ret_ok) return ret;
	}

	/* Print info
	 */
	cherokee_buffer_add_fsize (&tmp, downloader->content_length);
	cherokee_buffer_add_str   (&tmp, " of ");
	cherokee_buffer_add_fsize (&tmp, downloader->info.body_recv);

	if (! quiet) {
		fprintf (stderr, "\rDownloading: %s", tmp.buf);
		fflush(stderr);
	}

	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}


static ret_t
do_download__finish (cherokee_downloader_t *downloader, void *param)
{
	UNUSED (downloader);
	UNUSED (param);

	fprintf (stderr, "\n");
	return ret_ok;
}


static ret_t
do_download (cherokee_downloader_t *downloader)
{
	ret_t                        ret;
	cherokee_downloader_status_t status;
	cherokee_boolean_t           got_headers = false;
	cherokee_boolean_t           reading     = false;

	do_download__init(downloader,NULL);

	for (;;) {
		/* Do some real work
		 */
		ret = cherokee_downloader_step (downloader, NULL, NULL);

		cherokee_downloader_get_status(downloader, &status);
		TRACE(ENTRIES, "ret=%d status=%d\n", ret, status);

		switch (ret) {
		case ret_ok:
			if ( !reading && (status & downloader_status_post_sent)) {
				reading = true;
			}

			if ( (status & downloader_status_headers_received) && !got_headers) {
				do_download__has_headers (downloader, NULL);
				got_headers = true;
			}

			if (status & downloader_status_data_available) {
				do_download__read_body (downloader, NULL);
			}

			if (status & downloader_status_finished) {
				do_download__finish (downloader, NULL);
			}
			break;

		case ret_eagain:
			/* It's going on..
			 */
			break;

		case ret_eof_have_data:
			if ((status & downloader_status_headers_received) && !got_headers) {
				do_download__has_headers (downloader, NULL);
				got_headers = true;
			}
			if (status & downloader_status_data_available) {
				do_download__read_body (downloader, NULL);
			}
			/* fall down */

		case ret_eof:
			if (status & downloader_status_finished) {
				do_download__finish (downloader, NULL);
			}
			/* fall down */

		case ret_error:
			/* Finished or critical error
			 */
			return ret;

		default:
			SHOULDNT_HAPPEN;
			return ret_error;
		}
	}

	return ret_ok;
}


int
main (int argc, char **argv)
{
	int                    re;
	ret_t                  ret;
	cint_t                 val;
	cint_t                 param_num;
	cint_t                 long_index;
	cherokee_downloader_t *downloader;
	cherokee_buffer_t      proxy       = CHEROKEE_BUF_INIT;
	cuint_t                proxy_port;

	struct option long_options[] = {
		/* Options without arguments */
		{"help",          no_argument,       NULL, 'h'},
		{"version",       no_argument,       NULL, 'V'},
		{"quiet",         no_argument,       NULL, 'q'},
		{"save-headers",  no_argument,       NULL, 's'},
		{"header",        required_argument, NULL,  0 },
		{NULL, 0, NULL, 0}
	};

	/* Parse known parameters
	 */
	while ((val = getopt_long (argc, argv, "VshqO:", long_options, &long_index)) != -1) {
		switch (val) {
		case 'V':
			printf ("Cherokee Downloader %s\n"
			        "Written by Alvaro Lopez Ortega <alvaro@alobbs.com>\n\n"
			        "Copyright (C) 2001-2014 Alvaro Lopez Ortega.\n"
			        "This is free software; see the source for copying conditions.  There is NO\n"
			        "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n",
			        PACKAGE_VERSION);
			return EXIT_OK;

		case 'O':
			if (global_fd != UNSET_FD) {
				close (global_fd);
			}

			if ((strlen(optarg) == 1) && (optarg[0] == '-')) {
				global_fd = fileno(stdout);
			} else {
				global_fd = open (optarg, O_WRONLY | O_CREAT, 0644);
			}

			if (global_fd < 0) {
				PRINT_MSG ("ERROR: Can not open %s\n", optarg);
				return EXIT_ERROR;
			}
			break;

		case 0:
			break;

		case 'q':
			quiet = true;
			break;

		case 's':
			save_headers = true;
			break;

		case 'h':
		case '?':
		default:
			print_help();
			return EXIT_OK;
		}
	}

	/* The rest..
	 */
	param_num = argc - optind;

	if (param_num <= 0) {
		print_usage();
		return EXIT_OK;
	}

	/* Tracing and proxy discovering..
	 */
	cherokee_init();
	cget_find_proxy (&proxy, &proxy_port);

	for (val=optind; val<optind+param_num; val++) {
		cherokee_buffer_t url = CHEROKEE_BUF_INIT;

		/* Build the url buffer
		 */
		ret = cherokee_buffer_add_va (&url, "%s", argv[val]);
		if (ret != ret_ok)
			exit (EXIT_ERROR);

		/* Create the downloader object..
		 */
		ret = cherokee_downloader_new (&downloader);
		if (ret != ret_ok)
			exit (EXIT_ERROR);

		ret = cherokee_downloader_init(downloader);
		if (ret != ret_ok)
			exit (EXIT_ERROR);

		if (! cherokee_buffer_is_empty (&proxy)) {
			ret = cherokee_downloader_set_proxy (downloader, &proxy, proxy_port);
			if (ret != ret_ok)
				exit (EXIT_ERROR);
		}

		ret = cherokee_downloader_set_url (downloader, &url);
		if (ret != ret_ok)
			exit (EXIT_ERROR);

		ret = cherokee_downloader_connect (downloader);
		if (ret != ret_ok)
			exit (EXIT_ERROR);

		/* Download it!
		 */
		ret = do_download (downloader);
		if ((ret != ret_ok) && (ret != ret_eof)) {
			exit (EXIT_ERROR);
		}

		/* Free the objects..
		 */
		cherokee_buffer_mrproper (&url);
		cherokee_downloader_free (downloader);
	}

	/* Close the output file
	 */
	re = close (output_fd);
	if (re != 0)
		exit (EXIT_ERROR);

	cherokee_mrproper();
	return EXIT_OK;
}
