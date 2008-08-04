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

#include "common-internal.h"
#include "config_reader.h"
#include "util.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define ENTRIES "config"


static ret_t 
do_parse_file (cherokee_config_node_t *conf, const char *file)
{
	ret_t              ret;
	cherokee_buffer_t  buf = CHEROKEE_BUF_INIT;

	ret = cherokee_buffer_read_file (&buf, (char *)file);
	if (ret != ret_ok) return ret;

	ret = cherokee_config_reader_parse_string (conf, &buf);
	if (ret != ret_ok) goto error;

	cherokee_buffer_mrproper (&buf);
	return ret_ok;

error:
	cherokee_buffer_mrproper (&buf);
	return ret_error;
}


static ret_t
do_include (cherokee_config_node_t *conf, cherokee_buffer_t *path) 
{
	int         re;
	struct stat info;

	re = stat (path->buf, &info);
	if (re < 0) {
		PRINT_MSG ("Could not access '%s'\n", path->buf);
		return ret_error;
	}

	if (S_ISREG(info.st_mode)) {
		return do_parse_file (conf, path->buf);

	} else if (S_ISDIR(info.st_mode)) {
		DIR              *dir;
		struct dirent    *entry;
		int               entry_len;
		
		dir = opendir (path->buf);
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
			
			ret = cherokee_buffer_add_va (&full_new, "%s/%s", path->buf, entry->d_name);
			if (unlikely (ret != ret_ok)) return ret;

			ret = do_parse_file (conf, full_new.buf);
			if (ret != ret_ok) {
				cherokee_buffer_mrproper (&full_new);
				return ret;
			}

			cherokee_buffer_mrproper (&full_new);
		}
			
		closedir (dir);
		return ret_ok;
	} 
	
	SHOULDNT_HAPPEN;
	return ret_error;
}


static ret_t
check_config_node_sanity (cherokee_config_node_t *conf)
{
	cherokee_list_t *i, *j;
	int              re;

	/* Finds duplicate properties written in lower/upper-case
	 */
	cherokee_config_node_foreach (i, conf) 
	{
		cherokee_config_node_foreach (j, conf) 
		{
			if (i == j)
				continue;

			re = cherokee_buffer_case_cmp_buf (&CONFIG_NODE(i)->key, 
							   &CONFIG_NODE(j)->key);
			if (re == 0) {
				PRINT_ERROR ("ERROR: '%s' and '%s' as child of the same node.\n",
					     CONFIG_NODE(i)->key.buf, CONFIG_NODE(j)->key.buf);
				return ret_error;
			}
		}
	}

	return ret_ok;
}


ret_t 
cherokee_config_reader_parse_string (cherokee_config_node_t *conf, cherokee_buffer_t *buf)
{
	ret_t              ret;
	char              *eol;
	char              *begin;
	char              *equal;
	char              *tmp;
	char              *eof;
	cherokee_buffer_t  key = CHEROKEE_BUF_INIT;
	cherokee_buffer_t  val = CHEROKEE_BUF_INIT;

	eof = buf->buf + buf->len;

	begin = buf->buf;
	do {
		/* Skip whites at the begining
		 */
		while ((begin < eof) && 
		       ((*begin == ' ') || (*begin == '\t') || 
			(*begin == '\r') || (*begin == '\n'))) 
		{
			begin++;
		}

		/* Look for the EOL
		 */
		eol = cherokee_min_str (strchr(begin, '\n'), 
					strchr(begin, '\r'));

		if (eol == NULL) 
			break;

		/* Check that it's long enough
		 */
		if (eol - begin <= 4) {
			begin = eol + 1;
			continue;
		}
		*eol = '\0';

		/* Read the line 
		 */
		if (*begin != '#') {
			cuint_t val_len;

			equal = strstr (begin, " = ");
			if (equal == NULL) goto error;
		
			tmp = equal;

			/* Skip whites: end of the key
			 */
			while (*tmp == ' ') 
				tmp--;
			cherokee_buffer_add (&key, begin, (tmp + 1) - begin);
			
			tmp = equal + 3;
			while (*tmp == ' ')
				tmp++;

			/* Skip whites: end of the value
			 */
			val_len = strlen(tmp);
			while ((val_len >= 1) &&
			       (tmp[val_len-1] == ' '))
			{
				val_len--;
			}

			cherokee_buffer_add (&val, tmp, val_len);
			
			TRACE(ENTRIES, "'%s' => '%s'\n", key.buf, val.buf);
			
			ret = cherokee_config_node_add_buf (conf, &key, &val);
			if (ret != ret_ok) 
				goto error;
		}

		/* Next loop
		 */
		begin = eol + 1;

		cherokee_buffer_clean (&key);
		cherokee_buffer_clean (&val);

	} while (eol != NULL);
	
	cherokee_buffer_mrproper (&key);
	cherokee_buffer_mrproper (&val);

	/* Sanity check
	 */
	ret = check_config_node_sanity (conf);
	if (ret != ret_ok)
		return ret;
	
	return ret_ok;

error:
	PRINT_MSG ("Error parsing: %s\n", begin);

	cherokee_buffer_mrproper (&key);
	cherokee_buffer_mrproper (&val);
	return ret_error;
}


ret_t 
cherokee_config_reader_parse (cherokee_config_node_t *conf, cherokee_buffer_t *path)
{
	return do_include (conf, path);
}
