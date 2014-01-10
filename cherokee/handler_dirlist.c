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
#include "handler_dirlist.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <limits.h>

#ifdef HAVE_PWD_H
#  include <pwd.h>
#endif

#ifdef HAVE_GRP_H
#  include <grp.h>
#endif

#include "list.h"
#include "util.h"
#include "connection.h"
#include "connection-protected.h"
#include "server.h"
#include "server-protected.h"
#include "plugin_loader.h"
#include "icons.h"
#include "common.h"
#include "human_strcmp.h"
#include "match.h"

#define ICON_WEB_DIR_DEFAULT "/cherokee_icons"
#define ENTRIES "handler,dirlist"


struct file_entry {
	cherokee_list_t   list_node;
	struct stat       stat;
	struct stat       rstat;
	cherokee_buffer_t realpath;
	cuint_t           name_len;
	struct dirent     info;          /* It *must* be the last entry */
};
typedef struct file_entry file_entry_t;

struct file_match {
	cherokee_list_t    list_node;
	cherokee_buffer_t  filename;
	cherokee_boolean_t is_wildcard;
};
typedef struct file_match file_match_t;


/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (dirlist, http_get | http_options);


/* Private type
 */
static ret_t
file_match_new (file_match_t **file)
{
	file_match_t *n;

	n = (file_match_t *) malloc (sizeof(file_match_t));
	if (unlikely(n == NULL)) {
		return ret_nomem;
	}

	INIT_LIST_HEAD (&n->list_node);
	cherokee_buffer_init (&n->filename);
	n->is_wildcard = false;

	*file = n;
	return ret_ok;
}

static void
file_match_free (file_match_t *file)
{
	if (file == NULL)
		return;

	cherokee_buffer_mrproper (&file->filename);
	free (file);
}


/* Private type
 */
static ret_t
file_entry_new (file_entry_t **file, cuint_t extra_size)
{
	file_entry_t *n;

	n = (file_entry_t *) malloc (sizeof(file_entry_t) + extra_size);
	if (unlikely(n == NULL)) {
		return ret_nomem;
	}

	INIT_LIST_HEAD (&n->list_node);
	cherokee_buffer_init (&n->realpath);

	*file = n;
	return ret_ok;
}

static void
file_entry_free (file_entry_t *file)
{
	cherokee_buffer_mrproper (&file->realpath);
	free (file);
}


/* Methods implementation
 */
static ret_t
load_theme_load_file (cherokee_buffer_t *theme_path, const char *file, cherokee_buffer_t *output)
{
	cherokee_buffer_t path = CHEROKEE_BUF_INIT;

	cherokee_buffer_add_buffer (&path, theme_path);
	cherokee_buffer_add (&path, file, strlen(file));

	cherokee_buffer_clean (output);
	cherokee_buffer_read_file (output, path.buf);

	cherokee_buffer_mrproper (&path);
	return ret_ok;
}


static ret_t
parse_if (cherokee_buffer_t *buf, const char *if_entry, size_t len_entry, cherokee_boolean_t show)
{
	char              *begin;
	char              *end;
	cherokee_buffer_t  token = CHEROKEE_BUF_INIT;

	cherokee_buffer_add_str (&token, "%if ");
	cherokee_buffer_add (&token, if_entry, len_entry);
	cherokee_buffer_add_str (&token, "%");

	begin = strstr (buf->buf, token.buf);
	if (begin == NULL)
		goto error;

	end = strstr (begin, "%fi%");
	if (end == NULL)
		goto error;

	if (show) {
		cherokee_buffer_remove_chunk (buf, end - buf->buf, 4);
		cherokee_buffer_remove_chunk (buf, begin - buf->buf, token.len);
	} else {
		cherokee_buffer_remove_chunk (buf, begin - buf->buf, (end+4) - begin);
	}

	cherokee_buffer_mrproper (&token);
	return ret_ok;
error:
	cherokee_buffer_mrproper (&token);
	return ret_error;
}


static ret_t
parse_macros_in_buffer (cherokee_buffer_t *buf, cherokee_handler_dirlist_props_t *props)
{
#define STR_SZ(str)	str, CSZLEN(str)

	parse_if (buf, STR_SZ("size"),  props->show_size);
	parse_if (buf, STR_SZ("date"),  props->show_date);
	parse_if (buf, STR_SZ("user"),  props->show_user);
	parse_if (buf, STR_SZ("group"), props->show_group);
	parse_if (buf, STR_SZ("icon"),  props->show_icons);

#undef STR_SZ
	return ret_ok;
}


static ret_t
load_theme (cherokee_buffer_t *theme_path, cherokee_handler_dirlist_props_t *props)
{
	load_theme_load_file (theme_path, "header.html", &props->header);
	load_theme_load_file (theme_path, "footer.html", &props->footer);
	load_theme_load_file (theme_path, "entry.html", &props->entry);
	load_theme_load_file (theme_path, "theme.css", &props->css);

	/* Check it
	 */
	if ((cherokee_buffer_is_empty (&props->header) ||
	     cherokee_buffer_is_empty (&props->entry) ||
	     cherokee_buffer_is_empty (&props->footer)))
	    return ret_error;

	/* Parse conditional blocks
	 */
	parse_macros_in_buffer (&props->header, props);
	parse_macros_in_buffer (&props->footer, props);
	parse_macros_in_buffer (&props->entry,  props);

	return ret_ok;
}


ret_t
cherokee_handler_dirlist_props_free  (cherokee_handler_dirlist_props_t *props)
{
	cherokee_list_t *i, *tmp;

	list_for_each_safe (i, tmp, &props->notice_files) {
		file_match_free ((file_match_t*)i);
	}

	list_for_each_safe (i, tmp, &props->hidden_files) {
		file_match_free ((file_match_t*)i);
	}

	cherokee_buffer_mrproper (&props->header);
	cherokee_buffer_mrproper (&props->footer);
	cherokee_buffer_mrproper (&props->entry);
	cherokee_buffer_mrproper (&props->css);
	cherokee_buffer_mrproper (&props->icon_web_dir);

	return cherokee_handler_props_free_base (HANDLER_PROPS(props));
}


static ret_t
file_match_add_cb (char *entry, void *data)
{
	ret_t            ret;
	file_match_t    *new_match = NULL;
	cherokee_list_t *list      = LIST(data);

	ret = file_match_new (&new_match);
	if (unlikely ((ret != ret_ok) || (new_match == NULL)))
		return ret_error;

	if ((strchr (entry, '*')) || (strchr (entry, '?')))
	{
		new_match->is_wildcard = true;
	}

	cherokee_buffer_add (&new_match->filename, entry, strlen(entry));

	TRACE(ENTRIES, "Match file entry: '%s' (wildcard: %s)\n",
	      new_match->filename.buf,
	      new_match->is_wildcard ? "yes" : "no");

	cherokee_list_add_tail (&new_match->list_node, list);
	return ret_ok;
}


ret_t
cherokee_handler_dirlist_configure (cherokee_config_node_t   *conf,
                                    cherokee_server_t        *srv,
                                    cherokee_module_props_t **_props)
{
	ret_t                             ret;
	cherokee_list_t                  *i;
	cherokee_handler_dirlist_props_t *props;
	const char                       *theme      = NULL;
	cherokee_buffer_t                 theme_path = CHEROKEE_BUF_INIT;

	UNUSED(srv);

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_dirlist_props);

		cherokee_handler_props_init_base (HANDLER_PROPS(n),
			MODULE_PROPS_FREE(cherokee_handler_dirlist_props_free));

		n->show_size      = true;
		n->show_date      = true;
		n->show_user      = false;
		n->show_group     = false;
		n->show_icons     = true;
		n->show_symlinks  = true;
		n->redir_symlinks = false;

		n->show_hidden    = false;
		n->show_backup    = false;

		cherokee_buffer_init (&n->header);
		cherokee_buffer_init (&n->footer);
		cherokee_buffer_init (&n->entry);
		cherokee_buffer_init (&n->css);

		cherokee_buffer_init (&n->icon_web_dir);
		cherokee_buffer_add_str (&n->icon_web_dir, ICON_WEB_DIR_DEFAULT);

		INIT_LIST_HEAD (&n->notice_files);
		INIT_LIST_HEAD (&n->hidden_files);

		*_props = MODULE_PROPS(n);
	}

	props = PROP_DIRLIST(*_props);

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		/* Convert the properties
		 */
		if (equal_buf_str (&subconf->key, "size")) {
			ret = cherokee_atob (subconf->val.buf, &props->show_size);
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "date")) {
			ret = cherokee_atob (subconf->val.buf, &props->show_date);
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "user")) {
			ret = cherokee_atob (subconf->val.buf, &props->show_user);
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "group")) {
			ret = cherokee_atob (subconf->val.buf, &props->show_group);
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "symlinks")) {
			ret = cherokee_atob (subconf->val.buf, &props->show_symlinks);
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "redir_symlinks")) {
			ret = cherokee_atob (subconf->val.buf, &props->redir_symlinks);
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "hidden")) {
			ret = cherokee_atob (subconf->val.buf, &props->show_hidden);
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "backup")) {
			ret = cherokee_atob (subconf->val.buf, &props->show_backup);
			if (ret != ret_ok) return ret;

		} else if (equal_buf_str (&subconf->key, "theme")) {
			theme = subconf->val.buf;

		} else if (equal_buf_str (&subconf->key, "icon_dir")) {
			cherokee_buffer_clean (&props->icon_web_dir);
			cherokee_buffer_add_buffer (&props->icon_web_dir, &subconf->val);

		} else if (equal_buf_str (&subconf->key, "notice_files")) {
			ret = cherokee_config_node_read_list (subconf, NULL, file_match_add_cb, &props->notice_files);
			if (unlikely (ret != ret_ok))
				return ret;

		} else if (equal_buf_str (&subconf->key, "hidden_files")) {
			ret = cherokee_config_node_read_list (subconf, NULL, file_match_add_cb, &props->hidden_files);
			if (unlikely (ret != ret_ok))
				return ret;
		}
	}

	/* Load the theme
	 */
	if (theme == NULL)
		theme = "default";

	cherokee_buffer_add_buffer (&theme_path, &srv->themes_dir);
	cherokee_buffer_add_va     (&theme_path, "/%s/", theme);

	ret = load_theme (&theme_path, props);
	if (ret != ret_ok) {
		LOG_ERROR (CHEROKEE_ERROR_HANDLER_DIRLIST_THEME, theme, theme_path.buf);
	}
	cherokee_buffer_mrproper (&theme_path);
	return ret;
}


static cherokee_boolean_t
is_file_in_list (cherokee_list_t *list, char *filename, cuint_t len)
{
	ret_t            ret;
	cherokee_list_t *i;

	list_for_each (i, list) {
		file_match_t *file_match = ((file_match_t *)i);

		if (file_match->is_wildcard) {
			ret = cherokee_wildcard_match (file_match->filename.buf, filename);
			if (ret == ret_ok) {
				return true;
			}
			continue;
		}

		if (cherokee_buffer_cmp (&file_match->filename, filename, len) == 0) {
			return true;
		}
	}

	return false;
}


static ret_t
realpath_buf (cherokee_buffer_t *in,
              cherokee_buffer_t *resolved)
{
	char *re;

	cherokee_buffer_ensure_size (resolved, PATH_MAX);

	re = realpath (in->buf, resolved->buf);
	if (re == NULL) {
		return ret_error;
	}

	resolved->len = strlen (resolved->buf);
	return ret_ok;
}


static ret_t
generate_file_entry (cherokee_handler_dirlist_t  *dhdl,
                     DIR                         *dir,
                     cherokee_buffer_t           *path,
                     cherokee_buffer_t           *local_realpath,
                     file_entry_t               **ret_entry)
{
	int            re;
	ret_t          ret;
	file_entry_t  *n;
	char          *name;
	cuint_t        extra;
	struct dirent *entry;

#ifdef _PC_NAME_MAX
	extra = pathconf(path->buf, _PC_NAME_MAX);
#else
	extra = PATH_MAX;
#endif

	/* New object
	 */
	ret = file_entry_new (&n, path->len + extra + 3);
	if (unlikely (ret != ret_ok)) {
		return ret;
	}

	for (;;) {
		/* Read a new directory entry
		 */
		cherokee_readdir (dir, &n->info, &entry);
		if (entry == NULL) {
			file_entry_free (n);
			return ret_eof;
		}

		name = (char *)entry->d_name;
		n->name_len = strlen(name);

		/* Skip some files
		 */
		if ((! HDL_DIRLIST_PROP(dhdl)->show_hidden) &&
		    (name[0] == '.'))
			continue;

		if ((! HDL_DIRLIST_PROP(dhdl)->show_backup) &&
		    ((name[0] == '#') || (name[n->name_len-1] == '~')))
			continue;

		if (is_file_in_list (&HDL_DIRLIST_PROP(dhdl)->notice_files, name, n->name_len) ||
		    is_file_in_list (&HDL_DIRLIST_PROP(dhdl)->hidden_files, name, n->name_len))
			continue;

		/* Build the local path, stat and clean
		 */
		cherokee_buffer_add (path, name, n->name_len);

		/* Check the filename size
		 */
		if (n->name_len > dhdl->longest_filename)
			dhdl->longest_filename = n->name_len;

		/* Path
		 */
		re = cherokee_lstat (path->buf, &n->stat);
		if (re < 0) {
			cherokee_buffer_drop_ending (path, n->name_len);

			file_entry_free (n);
			return ret_error;
		}

		if (S_ISLNK(n->stat.st_mode)) {
			/* Info about the (target) linked file
			 */
			ret = cherokee_stat (path->buf, &n->rstat);
			if (ret != ret_ok) {
				/* Broken link */
				memcpy (&n->rstat, &n->stat, sizeof(struct stat));
			}

			if (HDL_DIRLIST_PROP(dhdl)->redir_symlinks) {
				/* The local directory realpath is build lazily
				 */
				if (cherokee_buffer_is_empty (local_realpath))
				{
					cherokee_buffer_drop_ending (path, n->name_len);

					ret = realpath_buf (path, local_realpath);
					if (unlikely (ret != ret_ok)) {
						file_entry_free (n);
						return ret_error;
					}

					cherokee_buffer_add (path, name, n->name_len);
				}

				/* Read the real path of the entry
				 */
				ret = realpath_buf (path, &n->realpath);
				if (unlikely (ret != ret_ok)) {
					cherokee_buffer_drop_ending (path, n->name_len);

					file_entry_free (n);
					return ret_error;
				}

				/* Check it is within the limits
				 */
				re = strncmp (n->realpath.buf, local_realpath->buf, local_realpath->len);
				if (re != 0) {
					cherokee_buffer_drop_ending (path, n->name_len);
					file_entry_free (n);
					return ret_error;
				}

				cherokee_buffer_move_to_begin (&n->realpath, local_realpath->len +1);
			}
		}

		/* Clean up and exit
		 */
		cherokee_buffer_drop_ending (path, n->name_len);

		*ret_entry = n;
		return ret_ok;
	}

	return ret_eof;
}


ret_t
cherokee_handler_dirlist_new (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props)
{
	ret_t              ret;
	cherokee_buffer_t *value;
	CHEROKEE_NEW_STRUCT (n, handler_dirlist);

	TRACE_CONN(cnt);

	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt, HANDLER_PROPS(props), PLUGIN_INFO_HANDLER_PTR(dirlist));

	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_dirlist_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_dirlist_free;
	HANDLER(n)->step        = (handler_func_step_t) cherokee_handler_dirlist_step;
	HANDLER(n)->add_headers = (handler_func_add_headers_t) cherokee_handler_dirlist_add_headers;

	/* Supported features
	 */
	HANDLER(n)->support     = hsupport_nothing;

	/* Process the request_string, and build the arguments table..
	 */
	cherokee_connection_parse_args (cnt);

	/* Init
	 */
	INIT_LIST_HEAD (&n->files);
	INIT_LIST_HEAD (&n->dirs);

	/* State
	 */
	n->dir_ptr          = NULL;
	n->file_ptr         = NULL;
	n->longest_filename = 0;

	/* Check if icons can be used
	 */
	if (HDL_DIRLIST_PROP(n)->show_icons) {
		HDL_DIRLIST_PROP(n)->show_icons = (HANDLER_SRV(n)->icons != NULL);
	}

	/* Choose the sorting key
	 */
	n->phase = dirlist_phase_add_header;
	n->sort  = Name_Down;

	ret = cherokee_avl_get_ptr (HANDLER_CONN(n)->arguments, "order", (void **) &value);
	if ((ret == ret_ok) && (value != NULL) && (value->len > 0)) {
		if      (value->buf[0] == 'N') n->sort = Name_Up;
		else if (value->buf[0] == 'n') n->sort = Name_Down;
		else if (value->buf[0] == 'D') n->sort = Date_Up;
		else if (value->buf[0] == 'd') n->sort = Date_Down;
		else if (value->buf[0] == 'S') n->sort = Size_Up;
		else if (value->buf[0] == 's') n->sort = Size_Down;
	}

	/* Properties
	 */
	cherokee_buffer_init (&n->header);
	cherokee_buffer_init (&n->public_dir);

	/* Check the theme
	 */
	if (cherokee_buffer_is_empty (&HDL_DIRLIST_PROP(n)->entry)  ||
	    cherokee_buffer_is_empty (&HDL_DIRLIST_PROP(n)->header) ||
	    cherokee_buffer_is_empty (&HDL_DIRLIST_PROP(n)->footer))
	{
		LOG_CRITICAL_S (CHEROKEE_ERROR_HANDLER_DIRLIST_BAD_THEME);
		cherokee_handler_free (HANDLER(n));
		return ret_error;
	}

	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t
cherokee_handler_dirlist_free (cherokee_handler_dirlist_t *dhdl)
{
	cherokee_list_t *i, *tmp;

	cherokee_buffer_mrproper (&dhdl->header);
	cherokee_buffer_mrproper (&dhdl->public_dir);

	list_for_each_safe (i, tmp, &dhdl->dirs) {
		cherokee_list_del (i);
		file_entry_free ((file_entry_t *)i);
	}

	list_for_each_safe (i, tmp, &dhdl->files) {
		cherokee_list_del (i);
		file_entry_free ((file_entry_t *)i);
	}

	return ret_ok;
}


static ret_t
check_request_finish_with_slash (cherokee_handler_dirlist_t *dhdl)
{
	cherokee_connection_t *conn = HANDLER_CONN(dhdl);

	if ((cherokee_buffer_is_empty (&conn->request)) ||
	    (!cherokee_buffer_is_ending (&conn->request, '/')))
	{
		cherokee_buffer_add_str (&conn->request, "/");
		cherokee_connection_set_redirect (conn, &conn->request);
		cherokee_buffer_drop_ending (&conn->request, 1);

		conn->error_code = http_moved_permanently;
		return ret_error;
	}

	return ret_ok;
}


static int
cmp_name_down (cherokee_list_t *a, cherokee_list_t *b)
{
	file_entry_t *f1 = (file_entry_t *)a;
	file_entry_t *f2 = (file_entry_t *)b;

	return cherokee_human_strcmp ((const char *) &f1->info.d_name,
	                              (const char *) &f2->info.d_name);
}


static int
cmp_size_down (cherokee_list_t *a, cherokee_list_t *b)
{
	file_entry_t *f1 = (file_entry_t *)a;
	file_entry_t *f2 = (file_entry_t *)b;

	if (f1->stat.st_size > f2->stat.st_size)
		return 1;

	if (f1->stat.st_size < f2->stat.st_size)
		return -1;

	return cmp_name_down (a,b);
}


static int
cmp_date_down (cherokee_list_t *a, cherokee_list_t *b)
{
	file_entry_t *f1 = (file_entry_t *)a;
	file_entry_t *f2 = (file_entry_t *)b;

	if (f1->stat.st_mtime > f2->stat.st_mtime)
		return 1;

	if (f1->stat.st_mtime < f2->stat.st_mtime)
		return -1;

	return cmp_name_down (a,b);
}


static int
cmp_name_up (cherokee_list_t *a, cherokee_list_t *b)
{
	return -cmp_name_down(a,b);
}


static int
cmp_size_up (cherokee_list_t *a, cherokee_list_t *b)
{
	return -cmp_size_down(a,b);
}


static int
cmp_date_up (cherokee_list_t *a, cherokee_list_t *b)
{
	return -cmp_date_down(a,b);
}


static void
list_sort_by_type (cherokee_list_t *list, cherokee_dirlist_sort_t sort)
{
	switch (sort) {
	case Name_Down:
		cherokee_list_sort (list, cmp_name_down);
		break;
	case Name_Up:
		cherokee_list_sort (list, cmp_name_up);
		break;
	case Size_Down:
		cherokee_list_sort (list, cmp_size_down);
		break;
	case Size_Up:
		cherokee_list_sort (list, cmp_size_up);
		break;
	case Date_Down:
		cherokee_list_sort (list, cmp_date_down);
		break;
	case Date_Up:
		cherokee_list_sort (list, cmp_date_up);
		break;
	}
}


static ret_t
build_file_list (cherokee_handler_dirlist_t *dhdl)
{
	DIR                   *dir;
	cherokee_connection_t *conn           = HANDLER_CONN(dhdl);
	cherokee_buffer_t      local_realpath = CHEROKEE_BUF_INIT;

	/* Build the local directory path
	 */
	cherokee_buffer_add_buffer (&conn->local_directory, &conn->request);     /* 1 */

	dir = cherokee_opendir (conn->local_directory.buf);
	if (dir == NULL) {
		conn->error_code = http_not_found;
		return ret_error;
	}

	/* Read the files
	 */
	for (;;) {
		ret_t ret;
		file_entry_t *item = NULL;
		int           is_dir;
		int           is_link;

		ret = generate_file_entry (dhdl, dir, &conn->local_directory, &local_realpath, &item);
		if (ret == ret_eof)
			break;
		if ((ret == ret_nomem) ||
		    (ret == ret_error) ||
		    (item == NULL))
			continue;

		is_link = S_ISLNK(item->stat.st_mode);
		if (is_link) {
			is_dir = S_ISDIR(item->rstat.st_mode);
		} else {
			is_dir = S_ISDIR(item->stat.st_mode);
		}

		if (is_dir) {
			cherokee_list_add (LIST(item), &dhdl->dirs);
		} else {
			cherokee_list_add (LIST(item), &dhdl->files);
		}
	}

	/* Clean
	 */
	cherokee_closedir(dir);
	cherokee_buffer_drop_ending (&conn->local_directory, conn->request.len); /* 2 */

	/* Free local_realpath. It might have been built lazily,
	 * inside the generate_file_entry() function.
	 */
	cherokee_buffer_mrproper (&local_realpath);

	/* Sort the file list
	 */
	if (! cherokee_list_empty(&dhdl->files)) {
		list_sort_by_type (&dhdl->files, dhdl->sort);
		dhdl->file_ptr = dhdl->files.next;
	}

	/* Sort the directories list:
	 * it doesn't make sense to look for the size
	 */
	if (! cherokee_list_empty (&dhdl->dirs)) {
		cherokee_dirlist_sort_t sort = dhdl->sort;

		if (sort == Size_Down) sort = Name_Down;
		if (sort == Size_Up)   sort = Name_Up;

		list_sort_by_type (&dhdl->dirs, sort);
		dhdl->dir_ptr = dhdl->dirs.next;
	}

	return ret_ok;
}


static ret_t
build_public_path (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buf)
{
	cherokee_connection_t *conn = HANDLER_CONN(dhdl);

	if (!cherokee_buffer_is_empty (&conn->userdir)) {
		/* ~user local dir request
		 */
		cherokee_buffer_add_str (buf, "/~");
		cherokee_buffer_add_buffer (buf, &conn->userdir);
	}

	if (cherokee_buffer_is_empty (&conn->request_original))
		cherokee_buffer_add_buffer (buf, &conn->request);
	else
		cherokee_buffer_add_buffer (buf, &conn->request_original);

	return ret_ok;
}


static ret_t
read_notice_file (cherokee_handler_dirlist_t *dhdl)
{
	ret_t                  ret;
	cherokee_list_t       *i;
	cherokee_connection_t *conn = HANDLER_CONN(dhdl);

	list_for_each (i, &HDL_DIRLIST_PROP(dhdl)->notice_files) {
		file_match_t      *file_match = ((file_match_t*)(i));
		cherokee_buffer_t *filename   = &file_match->filename;

		cherokee_buffer_clean (&dhdl->header);

		if (filename->buf[0] != '/') {
			cherokee_buffer_add_buffer (&conn->local_directory, &conn->request);                     /* do   */
			cherokee_buffer_add_buffer (&conn->local_directory, filename);

			ret = cherokee_buffer_read_file (&dhdl->header, conn->local_directory.buf);
			cherokee_buffer_drop_ending (&conn->local_directory, conn->request.len + filename->len); /* undo */
		} else {
			ret = cherokee_buffer_read_file (&dhdl->header, filename->buf);
		}

		if (ret == ret_ok)
			break;
	}

	return ret_ok;
}


ret_t
cherokee_handler_dirlist_init (cherokee_handler_dirlist_t *dhdl)
{
	ret_t ret;

	/* The request must end with a slash..
	 */
	ret = check_request_finish_with_slash (dhdl);
	if (ret != ret_ok)
		return ret;

	/* OPTIONS request: no need to build the file list
	 */
	if (HANDLER_CONN(dhdl)->header.method == http_options) {
		return ret_ok;
	}

	/* Read the Notice file
	 */
	if (! cherokee_list_empty (&HDL_DIRLIST_PROP(dhdl)->notice_files)) {
		ret = read_notice_file (dhdl);
		if (ret != ret_ok)
			return ret;
	}

	/* Build de local request
	 */
	ret = build_file_list (dhdl);
	if (unlikely(ret < ret_ok))
		return ret;

	/* Build public dir string
	 */
	ret = build_public_path (dhdl, &dhdl->public_dir);
	if (unlikely (ret != ret_ok))
		return ret;

	return ret_ok;
}


/* Substitute all instances of (token) found in vbuf[*pidx_buf];
 * if at least one (token) instance is found, then contents
 * resulting from substitution(s) are copied to the buffer
 * in next position of (vbuf) and (*pidx_buf) is updated to point
 * to current content buffer.
 * NOTE: only the first 2 positions in vbuf are used
 *       (flip/flop algorithm).
 */
static ret_t
substitute_vbuf_token (cherokee_buffer_t **vbuf,
                       size_t             *pidx_buf,
                       const char         *token,
                       int                 token_len,
                       const char         *replacement)
{
	ret_t ret;

	if (replacement == NULL)
		replacement = "";

	/* Substitute all instances of token in vbuf[*pidx_buf],
	 * if everything goes well then result is copied in next index position
	 * of vbuf[] and in this case we can increment the index position.
	 * NOTE: *pidx_buf ^= 1 is faster than *pidx_buf = (*pidx_buf + 1) % 2
	 */
	ret = cherokee_buffer_substitute_string (vbuf[*pidx_buf],
	                                         vbuf[*pidx_buf ^ 1],
	                                         (char *)token, token_len,
	                                         (char *)replacement, strlen(replacement));
	if (ret == ret_ok)
		*pidx_buf ^= 1;

	return ret;
}


/* Useful macros to declare, to initialize and to handle an array of
 * two flip/flop temporary buffers (used for substitution purposes).
 */

#define VTMP_INIT_SUBST(thread, vtmp, buffer_pattern) \
	vtmp[0] = THREAD_TMP_BUF1(thread);            \
	vtmp[1] = THREAD_TMP_BUF2(thread);            \
	cherokee_buffer_clean (vtmp[0]);              \
	cherokee_buffer_clean (vtmp[1]);              \
	cherokee_buffer_add_buffer (vtmp[0], (buffer_pattern))

#define VTMP_SUBSTITUTE_TOKEN(token, val)             \
	substitute_vbuf_token (vtmp, &idx_tmp, (token), sizeof(token)-1, (val))


static ret_t
render_file (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer, file_entry_t *file)
{
	ret_t                             ret;
	cherokee_buffer_t                *vtmp[2];
	cherokee_buffer_t                 name_buf;
	int                               is_dir;
	int                               is_link;
	const char                       *alt      = NULL;
	cherokee_buffer_t                *icon     = NULL;
	const char                       *name     = (char *) &file->info.d_name;
	cherokee_icons_t                 *icons    = HANDLER_SRV(dhdl)->icons;
	cherokee_buffer_t                *tmp      = &dhdl->header;
	cherokee_handler_dirlist_props_t *props    = HDL_DIRLIST_PROP(dhdl);
	cherokee_thread_t                *thread   = HANDLER_THREAD(dhdl);
	size_t                            idx_tmp  = 0;

	/* Initialize temporary substitution buffers
	 */
	VTMP_INIT_SUBST (thread, vtmp, &props->entry);

	is_link = S_ISLNK(file->stat.st_mode);
	if (is_link) {
		is_dir = S_ISDIR(file->rstat.st_mode);
	} else {
		is_dir = S_ISDIR(file->stat.st_mode);
	}

	/* Check whether it is a symlink that we should skip
	 */
	if (is_link) {
		if (! props->show_symlinks) {
			return ret_not_found;
		}
	}

	/* Name buffer
	 */
	cherokee_buffer_fake (&name_buf, name, strlen(name));

	/* Add the icon
	 */
	if (props->show_icons) {
		if (is_dir) {
			icon = &icons->directory_icon;
		} else {
			ret = cherokee_icons_get_icon (icons, &name_buf, &icon);
			if (ret != ret_ok)
				return ret;
		}
	}

	/* Icon's 'alt'
	 */
	alt = (is_dir) ? "[DIR]" : "[   ]";
	VTMP_SUBSTITUTE_TOKEN ("%icon_alt%", alt);
	VTMP_SUBSTITUTE_TOKEN ("%icon_dir%", props->icon_web_dir.buf);

	if (icon) {
		cherokee_buffer_clean (tmp);
		cherokee_buffer_add_buffer (tmp, &props->icon_web_dir);
		cherokee_buffer_add_char (tmp, '/');
		cherokee_buffer_add_buffer (tmp, icon);
		VTMP_SUBSTITUTE_TOKEN ("%icon%", tmp->buf);
	} else {
		VTMP_SUBSTITUTE_TOKEN ("%icon%", NULL);
	}

	/* File
	 */
	cherokee_buffer_clean (tmp);
	cherokee_buffer_add_escape_html (tmp, &name_buf);
	VTMP_SUBSTITUTE_TOKEN ("%file_name%", tmp->buf);

	if ((is_link) && (props->redir_symlinks)) {
		if (file->realpath.len <= 0) {
			return ret_not_found;
		}

		cherokee_buffer_clean (tmp);
		cherokee_buffer_escape_uri_delims (tmp, &file->realpath);
		VTMP_SUBSTITUTE_TOKEN ("%file_link%", tmp->buf);

	} else if (! is_dir) {
		cherokee_buffer_clean (tmp);
		cherokee_buffer_escape_uri_delims (tmp, &name_buf);
		VTMP_SUBSTITUTE_TOKEN ("%file_link%", tmp->buf);

	} else {
		cherokee_buffer_clean (tmp);
		cherokee_buffer_escape_uri_delims (tmp, &name_buf);
		cherokee_buffer_add_str (tmp, "/");
		VTMP_SUBSTITUTE_TOKEN ("%file_link%", tmp->buf);
	}

	/* Date
	 */
	if (props->show_date) {
		struct tm *ltime;
		struct tm  ltime_buf;

		cherokee_buffer_clean (tmp);
		cherokee_buffer_ensure_size (tmp, 33);

		ltime = cherokee_localtime (&file->stat.st_mtime, &ltime_buf);
		if (ltime != NULL) {
			strftime (tmp->buf, 32, "%d-%b-%Y %H:%M", ltime);
		}

		VTMP_SUBSTITUTE_TOKEN ("%date%", tmp->buf);
	}

	/* Size
	 */
	if (props->show_size) {
		if (is_link) {
			VTMP_SUBSTITUTE_TOKEN ("%size_unit%", NULL);
			VTMP_SUBSTITUTE_TOKEN ("%size%", "link");
		} else if (is_dir) {
			VTMP_SUBSTITUTE_TOKEN ("%size_unit%", NULL);
			VTMP_SUBSTITUTE_TOKEN ("%size%", "-");
		} else {
			char *unit;

			cherokee_buffer_clean (tmp);
			cherokee_buffer_ensure_size (tmp, 8);
			cherokee_buffer_add_fsize (tmp, file->stat.st_size);

			unit = tmp->buf;
			while ((*unit >= '0')  && (*unit <= '9')) unit++;

			VTMP_SUBSTITUTE_TOKEN ("%size_unit%", unit);
			*unit = '\0';
			VTMP_SUBSTITUTE_TOKEN ("%size%", tmp->buf);
		}
	}

	/* User
	 */
	if (props->show_user) {
		struct passwd *user;
		const char    *name;

		user = getpwuid (file->stat.st_uid);
		name = (char *) (user->pw_name) ? user->pw_name : "unknown";

		VTMP_SUBSTITUTE_TOKEN ("%user%", name);
	}

	/* Group
	 */
	if (props->show_group) {
		struct group *user;
		const char   *group;

		user = getgrgid (file->stat.st_gid);
		group = (char *) (user->gr_name) ? user->gr_name : "unknown";

		VTMP_SUBSTITUTE_TOKEN ("%group%", group);
	}

	/* Add final result to buffer
	 */
	cherokee_buffer_add_buffer (buffer, vtmp[idx_tmp]);

	return ret_ok;
}


static ret_t
render_parent_directory (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer)
{
	cherokee_buffer_t                *vtmp[2];
	cherokee_buffer_t                *icon     = NULL;
	cherokee_icons_t                 *icons    = HANDLER_SRV(dhdl)->icons;
	cherokee_handler_dirlist_props_t *props    = HDL_DIRLIST_PROP(dhdl);
	cherokee_thread_t                *thread   = HANDLER_THREAD(dhdl);
	cherokee_buffer_t                *tmp      = &dhdl->header;
	size_t                            idx_tmp  = 0;

	/* Initialize temporary substitution buffers
	 */
	VTMP_INIT_SUBST (thread, vtmp, &props->entry);

	if (props->show_icons) {
		icon = &icons->parentdir_icon;
	}

	if (icon) {
		cherokee_buffer_clean (tmp);
		cherokee_buffer_add_buffer (tmp, &props->icon_web_dir);
		cherokee_buffer_add_char (tmp, '/');
		cherokee_buffer_add_buffer (tmp, icon);
		VTMP_SUBSTITUTE_TOKEN ("%icon%", tmp->buf);
	} else {
		VTMP_SUBSTITUTE_TOKEN ("%icon%", NULL);
	}

	VTMP_SUBSTITUTE_TOKEN ("%icon_alt%", "[DIR]");
	VTMP_SUBSTITUTE_TOKEN ("%icon_dir%", props->icon_web_dir.buf);

	VTMP_SUBSTITUTE_TOKEN ("%file_link%", "../");
	VTMP_SUBSTITUTE_TOKEN ("%file_name%", "Parent Directory");

	VTMP_SUBSTITUTE_TOKEN ("%date%", NULL);
	VTMP_SUBSTITUTE_TOKEN ("%size_unit%", NULL);
	VTMP_SUBSTITUTE_TOKEN ("%size%", "-");
	VTMP_SUBSTITUTE_TOKEN ("%user%", NULL);
	VTMP_SUBSTITUTE_TOKEN ("%group%", NULL);

	/* Add final result to buffer
	 */
	cherokee_buffer_add_buffer (buffer, vtmp[idx_tmp]);

	return ret_ok;
}


static ret_t
render_header_footer_vbles (cherokee_handler_dirlist_t *dhdl,
                            cherokee_buffer_t          *buffer,
                            cherokee_buffer_t          *buf_pattern)
{
	cherokee_buffer_t                *vtmp[2];
	cherokee_thread_t                *thread   = HANDLER_THREAD(dhdl);
	size_t                            idx_tmp  = 0;
	cherokee_bind_t                  *bind     = CONN_BIND(HANDLER_CONN(dhdl));
	cherokee_handler_dirlist_props_t *props    = HDL_DIRLIST_PROP(dhdl);

	/* Initialize temporary substitution buffers
	 */
	VTMP_INIT_SUBST (thread, vtmp, buf_pattern);

	/* Replacements
	 */
	VTMP_SUBSTITUTE_TOKEN ("%public_dir%",      dhdl->public_dir.buf);
	VTMP_SUBSTITUTE_TOKEN ("%server_software%", bind->server_string_w_port.buf);
	VTMP_SUBSTITUTE_TOKEN ("%notice%",          dhdl->header.buf);
	VTMP_SUBSTITUTE_TOKEN ("%icon_dir%",        props->icon_web_dir.buf);

	/* Orders
	 */
	VTMP_SUBSTITUTE_TOKEN ("%order_name%", (dhdl->sort == Name_Down) ? "N" : "n");
	VTMP_SUBSTITUTE_TOKEN ("%order_size%", (dhdl->sort == Size_Down) ? "S" : "s");
	VTMP_SUBSTITUTE_TOKEN ("%order_date%", (dhdl->sort == Date_Down) ? "D" : "d");

	/* Add final result to buffer
	 */
	cherokee_buffer_add_buffer (buffer, vtmp[idx_tmp]);

	return ret_ok;
}


ret_t
cherokee_handler_dirlist_step (cherokee_handler_dirlist_t *dhdl,
                               cherokee_buffer_t          *buffer)
{
	ret_t                             ret = ret_ok;
	cherokee_handler_dirlist_props_t *props = HDL_DIRLIST_PROP(dhdl);

	/* OPTIONS request
	 */
	if (HANDLER_CONN(dhdl)->header.method == http_options) {
		return ret_eof;
	}

	/* GET request
	 */
	switch (dhdl->phase) {
	case dirlist_phase_add_header:
		/* Add the theme header
		 */
		ret = render_header_footer_vbles (dhdl, buffer, &props->header);
		if (unlikely (ret != ret_ok)) return ret;
		if (buffer->len > DEFAULT_READ_SIZE)
			return ret_ok;
		dhdl->phase = dirlist_phase_add_parent_dir;

	case dirlist_phase_add_parent_dir:
		ret = render_parent_directory (dhdl, buffer);
		if (unlikely (ret != ret_ok)) return ret;
		dhdl->phase = dirlist_phase_add_entries;

	case dirlist_phase_add_entries:
		/* Print the directories first
		 */
		while (dhdl->dir_ptr) {
			if (dhdl->dir_ptr == &dhdl->dirs) {
				dhdl->dir_ptr = NULL;
				break;
			}
			render_file (dhdl, buffer, (file_entry_t *)dhdl->dir_ptr);
			dhdl->dir_ptr = dhdl->dir_ptr->next;

			/* Maybe it has read enough data
			 */
			if (buffer->len > DEFAULT_READ_SIZE)
				return ret_ok;
		}

		/* then the file list
		 */
		while (dhdl->file_ptr) {
			if (dhdl->file_ptr == &dhdl->files) {
				dhdl->file_ptr = NULL;
				break;
			}
			render_file (dhdl, buffer, (file_entry_t *) dhdl->file_ptr);
			dhdl->file_ptr = dhdl->file_ptr->next;

			/* Maybe it has read enough data
			 */
			if (buffer->len > DEFAULT_READ_SIZE)
				return ret_ok;
		}
		dhdl->phase = dirlist_phase_add_footer;

	case dirlist_phase_add_footer:
		/* Add the theme footer
		 */
		ret = render_header_footer_vbles (dhdl, buffer, &props->footer);
		if (unlikely (ret != ret_ok)) return ret;

		dhdl->phase = dirlist_phase_finished;
		return ret_eof_have_data;

	case dirlist_phase_finished:
		break;
	}

	return ret_eof;
}


ret_t
cherokee_handler_dirlist_add_headers (cherokee_handler_dirlist_t *dhdl,
                                      cherokee_buffer_t          *buffer)
{
	UNUSED(dhdl);

	/* OPTIONS request
	 */
	if (HANDLER_CONN(dhdl)->header.method == http_options) {
		cherokee_buffer_add_str (buffer, "Content-Length: 0"CRLF);
		cherokee_handler_add_header_options (HANDLER(dhdl), buffer);
		return ret_ok;
	}

	/* GET request
	 */
	cherokee_buffer_add_str (buffer, "Content-Type: text/html; charset=utf-8"CRLF);
	return ret_ok;
}
