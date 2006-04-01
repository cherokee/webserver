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
#include <sys/types.h>
#include <dirent.h>

#include "module_read_config.h"
#include "module_loader.h"
#include "server.h"
#include "server-protected.h"
#include "list_ext.h"
#include "mime.h"
#include "util.h"


cherokee_module_info_t MODULE_INFO(read_config) = {
	cherokee_generic,                  /* type     */
	cherokee_module_read_config_new    /* new func */
};



ret_t 
cherokee_module_read_config_new  (cherokee_module_read_config_t **config)
{
	CHEROKEE_NEW_STRUCT(n,module_read_config);

	cherokee_module_init_base (MODULE(n));

	MODULE(n)->instance = (module_func_new_t)  cherokee_module_read_config_new;
	MODULE(n)->free     = (module_func_free_t) cherokee_module_read_config_free;

	*config = n;
	return ret_ok;
}


ret_t 
cherokee_module_read_config_free (cherokee_module_read_config_t *config)
{
	free (config);
	return ret_ok;
}


static ret_t
read_inclusion (char *path, cherokee_buffer_t *buf)
{
	int         re;
	struct stat info;

	re = stat (path, &info);
	if (re < 0) {
		PRINT_MSG ("Could not access '%s'\n", path);
		return ret_error;
	}
			
	if (S_ISREG(info.st_mode)) {
		cherokee_buffer_read_file (buf, path);
		return ret_ok;

	} else if (S_ISDIR(info.st_mode)) {
		DIR              *dir;
		struct dirent    *entry;
		int               entry_len;
		
		dir = opendir (path);
		if (dir == NULL) return ret_error;
		
		while ((entry = readdir(dir)) != NULL) {
			ret_t             ret;
			cherokee_buffer_t full_new = CHEROKEE_BUF_INIT;
			
			/* Ignore backup files
			 */
			entry_len = strlen(entry->d_name);
			
			if ((entry->d_name[0] == '.') ||
			    (entry->d_name[0] == '#') ||
			    (entry->d_name[entry_len-1] == '~'))
			{
				continue;
			}
			
			ret = cherokee_buffer_add_va (&full_new, "%s/%s", path, entry->d_name);
			if (unlikely (ret != ret_ok)) return ret;

			cherokee_buffer_read_file (buf, full_new.buf);
			cherokee_buffer_mrproper (&full_new);
		}
			
		closedir (dir);
		return ret_ok;
	} 
	
	SHOULDNT_HAPPEN;
	return ret_error;
}


static cherokee_boolean_t
current_line_is_comment (char *beginning, char *pos)
{
	char *p;

	for (p = pos; p >= beginning; p--) {
		if (*p == '#') return true;
		if ((*p == '\r') || (*p == '\n')) return false;
	}

	return false;
}


static ret_t
replace_inclusions (cherokee_buffer_t *config)
{
	ret_t  ret;
	char  *incl;
	char  *nl, *nl1, *nl2, ch;
	cherokee_buffer_t tmp      = CHEROKEE_BUF_INIT;
	cherokee_buffer_t tmp_incl = CHEROKEE_BUF_INIT;
	

	for (;;) {		
		char *p = config->buf;
		
		/* Look for the next suitable Include entry
		 */
		for (;;) {
			incl = strcasestr (p, "Include ");
			if (incl == NULL) goto out;

			if (!current_line_is_comment (config->buf, incl))
				break;

			p += 8;
		}
		
		/* Parse it
		 */
		nl1 = strchr (incl, '\r');
		nl2 = strchr (incl, '\n');

		nl = cherokee_min_str (nl1, nl2);
		if (nl == NULL) return ret_error;

		cherokee_buffer_add (&tmp_incl, incl, nl - incl);

		incl += 8;
		while ((*incl == ' ') || (*incl == '\t')) incl++;

		/* Replace its content
		 */
		ch = *nl;
		*nl = '\0';
		ret = read_inclusion (incl, &tmp);
		*nl = ch;			

		if (ret != ret_ok) 
			goto error;
		
		cherokee_buffer_replace_string (config, tmp_incl.buf, tmp_incl.len, tmp.buf, tmp.len);
		cherokee_buffer_clean (&tmp);
		cherokee_buffer_clean (&tmp_incl);
	}

out:	
	cherokee_buffer_mrproper (&tmp);
	cherokee_buffer_mrproper (&tmp_incl);

	return ret_ok;

error:
	cherokee_buffer_mrproper (&tmp);
	cherokee_buffer_mrproper (&tmp_incl);

	return ret_error;
}


CHEROKEE_EXPORT ret_t
read_config_file (cherokee_server_t *srv, char *filename)
{
	ret_t             ret;
	cherokee_buffer_t config = CHEROKEE_BUF_INIT;
	
	ret = cherokee_buffer_read_file (&config, filename);
	if (ret != ret_ok) goto go_out;

	ret = replace_inclusions (&config);
	if (ret != ret_ok) goto go_out;

	ret = read_config_string (srv, config.buf);

go_out:
	cherokee_buffer_mrproper (&config);
	return ret;
}


ret_t 
read_config_string (cherokee_server_t *srv, char *config_string)
{
	ret_t   ret;
	int     error;
	void   *bufstate;

	extern int  yyparse             (void *);
	extern int  yy_scan_string      (const char *);
	extern void yy_switch_to_buffer (void *);
	extern void yy_delete_buffer    (void *);

	bufstate = (void *) yy_scan_string (config_string);
	yy_switch_to_buffer(bufstate);

	error = yyparse((void *)srv);

	yy_delete_buffer (bufstate);

	ret = (error == 0) ? ret_ok : ret_error;
	if (ret < ret_ok) {
		return ret;
	}
	
#ifndef CHEROKEE_EMBEDDED
	/* Maybe read the Icons file
	 */
	if (srv->icons_file != NULL) {
		ret = cherokee_icons_read_config_file (srv->icons, srv->icons_file);
		if (ret < ret_ok) {
			PRINT_ERROR_S ("Cannot read the icons file\n");
		}
	}

	/* Maybe read the MIME file
	 */
	if (srv->mime_file != NULL) {
		if (srv->mime == NULL) {
			ret = cherokee_mime_new (&srv->mime);
			if (ret < ret_ok) {
				PRINT_ERROR_S ("Can not get default MIME configuration file\n");
				return ret;
			}
		}

		ret = cherokee_mime_load_mime_types (srv->mime, srv->mime_file);
		if (ret < ret_ok) {
			PRINT_ERROR ("Can not load MIME configuration file %s\n", srv->mime_file);
			return ret;
		}
	}
#endif

	return ret;
}




/* Library init function
 */
void
MODULE_INIT(read_config) (cherokee_module_loader_t *loader)
{
}
