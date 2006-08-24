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
#include <sys/stat.h>
#include <unistd.h>

#include "config_node.h"
#include "util.h"
#include "list_ext.h"

#define ENTRIES "config"


ret_t 
cherokee_config_node_init (cherokee_config_node_t *conf)
{
	INIT_LIST_HEAD (&conf->entry);
	INIT_LIST_HEAD (&conf->child);
	
	cherokee_buffer_init (&conf->key);
	cherokee_buffer_init (&conf->val);
	
	return ret_ok;
}

ret_t 
cherokee_config_node_new (cherokee_config_node_t **conf)
{
	CHEROKEE_NEW_STRUCT(n,config_node);

	cherokee_config_node_init (n);

	*conf = n;
	return ret_ok;
}


ret_t 
cherokee_config_node_mrproper (cherokee_config_node_t *conf)
{
	list_t *i, *j;

	cherokee_buffer_mrproper (&conf->key);
	cherokee_buffer_mrproper (&conf->val);

	list_for_each_safe (i, j, &conf->child) {
		cherokee_config_node_free (CONFIG_NODE(i));
	}

	return ret_ok;
}

ret_t 
cherokee_config_node_free (cherokee_config_node_t *conf)
{
	cherokee_config_node_mrproper (conf);

	free (conf);
	return ret_ok;
}


static cherokee_config_node_t *
search_child (cherokee_config_node_t *current, cherokee_buffer_t *child)
{
	list_t                        *i;
	cherokee_config_node_t *entry;

	list_for_each (i, &current->child) {
		entry = CONFIG_NODE(i);

		if (strcmp (entry->key.buf, child->buf) == 0)
			return entry;
	}

	return NULL;
}


static cherokee_config_node_t *
add_new_child (cherokee_config_node_t *entry, cherokee_buffer_t *key)
{
	ret_t                   ret;
	cherokee_config_node_t *n;

	ret = cherokee_config_node_new (&n);
	if (ret != ret_ok) return NULL;

	cherokee_buffer_add_buffer (&n->key, key);	   

	list_add_tail ((list_t *)n, &entry->child);
	return n;
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
		return cherokee_config_node_parse_file (conf, path->buf);

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

			ret = cherokee_config_node_parse_file (conf, full_new.buf);
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


ret_t 
cherokee_config_node_add (cherokee_config_node_t *conf, const char *key, cherokee_buffer_t *val)
{
	char                   *sep;
	cherokee_config_node_t *child;
	cherokee_config_node_t *current = conf;
	const char             *begin   = key;
	cherokee_buffer_t       tmp     = CHEROKEE_BUF_INIT;
	cherokee_boolean_t      final   = false;
	
	/* 'include' is a special case
	 */
	if (equal_str (key, "include")) {
		return do_include (conf, val);
	}

	do {
		/* Extract current child
		 */
		sep = strchr (begin, '!');
		if (sep != NULL) {
			cherokee_buffer_add (&tmp, (char *)begin, sep - begin);
		} else {
			cherokee_buffer_add (&tmp, (char *)begin, strlen(begin));
			final = true;
		}

		/* Look for the child entry
		 */
		child = search_child (current, &tmp);
		if (child == NULL) {
			child = add_new_child (current, &tmp);

			if (final) {
				cherokee_buffer_add_buffer (&child->val, val);
			}
		}

		/* Prepare for next step
		 */
		begin = sep + 1;
		current = child;

		cherokee_buffer_clean (&tmp);

	} while (! final);

	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}


ret_t 
cherokee_config_node_add_buf (cherokee_config_node_t *conf, cherokee_buffer_t *key, cherokee_buffer_t *val)
{
	return cherokee_config_node_add (conf, key->buf, val);
}


ret_t
cherokee_config_node_get (cherokee_config_node_t *conf, const char *key, cherokee_config_node_t **entry)
{
	char                   *sep;
	cherokee_config_node_t *child;
	cherokee_config_node_t *current = conf;
	const char             *begin   = key;
	cherokee_buffer_t       tmp     = CHEROKEE_BUF_INIT;
	cherokee_boolean_t      final   = false;

	do {
		/* Extract current child
		 */
		sep = strchr (begin, '!');
		if (sep != NULL) {
			cherokee_buffer_add (&tmp, (char *)begin, sep - begin);
		} else {
			cherokee_buffer_add (&tmp, (char *)begin, strlen(begin));
			final = true;
		}

		/* Look for the child entry
		 */
		child = search_child (current, &tmp);
		if (child == NULL) {
			cherokee_buffer_mrproper (&tmp);
			return ret_not_found;
		}

		if (final) {
			*entry = child;
		}

		/* Prepare for next step
		 */
		begin = sep + 1;
		current = child;

		cherokee_buffer_clean (&tmp);

	} while (! final);
       
	cherokee_buffer_mrproper (&tmp);
	return ret_ok;
}


ret_t 
cherokee_config_node_get_buf (cherokee_config_node_t *conf, cherokee_buffer_t *key, cherokee_config_node_t **entry)
{
	return cherokee_config_node_get (conf, key->buf, entry);
}


ret_t 
cherokee_config_node_while (cherokee_config_node_t *conf, cherokee_config_node_while_func_t func, void *data)
{
	ret_t   ret;
	list_t *i;

	cherokee_config_node_foreach (i, conf) {
		ret = func (CONFIG_NODE(i), data);
		if (ret != ret_ok) return ret;
	}

	return ret_ok;
}


ret_t 
cherokee_config_node_parse_string (cherokee_config_node_t *conf, cherokee_buffer_t *buf)
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

		/* Mark the EOL
		 */
		eol = cherokee_min_str (strchr(begin, '\n'), 
					strchr(begin, '\r'));

		if (eol == NULL) 
			break;

		if (eol - begin <= 4) {
			begin = eol + 1;
			continue;
		}
		*eol = '\0';

		/* Read the line 
		 */
		if (*begin != '#') {
			equal = strstr (begin, " = ");
			if (equal == NULL) goto error;
		
			tmp = equal;
			while (*tmp == ' ') tmp--;
			cherokee_buffer_add (&key, begin, (tmp + 1) - begin);
			
			tmp = equal + 3;
			while (*tmp == ' ') tmp++;		
			cherokee_buffer_add (&val, tmp, strlen(tmp));

			TRACE(ENTRIES, "'%s' => '%s'\n", key.buf, val.buf);

			ret = cherokee_config_node_add_buf (conf, &key, &val);
			if (ret != ret_ok) goto error;
		}

		/* Next loop
		 */
		begin = eol + 1;

		cherokee_buffer_clean (&key);
		cherokee_buffer_clean (&val);

	} while (eol != NULL);
	
	cherokee_buffer_mrproper (&key);
	cherokee_buffer_mrproper (&val);
	return ret_ok;

error:
	PRINT_MSG ("Error parsing: %s\n", begin);

	cherokee_buffer_mrproper (&key);
	cherokee_buffer_mrproper (&val);
	return ret_error;
}


ret_t 
cherokee_config_node_parse_file (cherokee_config_node_t *conf, const char *file)
{
	ret_t              ret;
	cherokee_buffer_t  buf = CHEROKEE_BUF_INIT;

	ret = cherokee_buffer_read_file (&buf, (char *)file);
	if (ret != ret_ok) return ret;

	ret = cherokee_config_node_parse_string (conf, &buf);
	if (ret != ret_ok) goto error;

	cherokee_buffer_mrproper (&buf);
	return ret_ok;

error:
	cherokee_buffer_mrproper (&buf);
	return ret_error;
}


ret_t 
cherokee_config_node_read (cherokee_config_node_t *conf, const char *key, cherokee_buffer_t **buf)
{
	ret_t                   ret;
	cherokee_config_node_t *tmp;

	ret = cherokee_config_node_get (conf, key, &tmp);
	if (ret != ret_ok) return ret;

	*buf = &tmp->val;
	return ret_ok;
}


ret_t 
cherokee_config_node_read_int  (cherokee_config_node_t *conf, const char *key, int *num)
{
	ret_t                   ret;
	cherokee_config_node_t *tmp;

	ret = cherokee_config_node_get (conf, key, &tmp);
	if (ret != ret_ok) return ret;

	*num = atoi (tmp->val.buf);
	return ret_ok;
}


ret_t 
cherokee_config_node_read_path (cherokee_config_node_t *conf, const char *key, cherokee_buffer_t **buf)
{
	ret_t                   ret;
	cherokee_config_node_t *tmp;

	if (key != NULL) {
		ret = cherokee_config_node_get (conf, key, &tmp);
		if (ret != ret_ok) return ret;
	} else {
		tmp = conf;
	}

	if (cherokee_buffer_end_char (&tmp->val) != '/')
		cherokee_buffer_add_str (&tmp->val, "/");

	*buf = &tmp->val;
	return ret_ok;
}


ret_t 
cherokee_config_node_read_list (cherokee_config_node_t           *conf, 
				const char                       *key, 
				cherokee_config_node_list_func_t  func, 
				void                             *param)
{
	ret_t                   ret;
	char                   *ptr;
	char                   *stop;
	cherokee_config_node_t *tmp;

	if (key != NULL) {
		ret = cherokee_config_node_get (conf, key, &tmp);
		if (ret != ret_ok) return ret;
	} else {
		tmp = conf;
	}

	ptr = tmp->val.buf;

	if (ptr == NULL)
		return ret_not_found;

	for (;;) {
		stop = strchr (ptr, ',');
		if (stop != NULL) *stop = '\0';
	
		ret = func (ptr, param);
		if (ret != ret_ok) return ret;
		
		if (stop == NULL)
			break;
		
		*stop = ',';
		ptr = stop + 1;
	}
	
	return ret_ok;
}


static ret_t
convert_to_list_step (char *entry, void *data)
{
	return cherokee_list_add_tail ((list_t *)data, strdup(entry));
}


ret_t
cherokee_config_node_convert_list (cherokee_config_node_t *conf, const char *key, list_t *list)
{
	return cherokee_config_node_read_list (conf, key, convert_to_list_step, list);
}
