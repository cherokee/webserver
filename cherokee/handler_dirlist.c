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
#include "handler_dirlist.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

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

#define DEFAULT_NAME_LEN 40


struct file_entry {
	cherokee_list_t  list_entry;
	struct stat      stat;
	cuint_t          name_len;
	struct dirent    info;          /* It *must* be the last entry */
};
typedef struct file_entry file_entry_t;


/* Plug-in initialization
 */
PLUGIN_INFO_HANDLER_EASIEST_INIT (dirlist, http_get);


/* Methods implementation
 */
static ret_t
load_theme_load_file (cherokee_buffer_t *theme_path, char *file, cherokee_buffer_t *output)
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
parse_if (cherokee_buffer_t *buf, char *if_entry, cherokee_boolean_t show)
{
	char              *begin;
	char              *end;
	cherokee_buffer_t  token = CHEROKEE_BUF_INIT;
	
	cherokee_buffer_add_str (&token, "%if ");
	cherokee_buffer_add (&token, if_entry, strlen(if_entry));
	cherokee_buffer_add_str (&token, "%");

	begin = strstr (buf->buf, token.buf);
	if (begin == NULL) goto error;
	
	end = strstr (begin, "%fi%");
	if (end == NULL) goto error;

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
	parse_if (buf, "size",  props->show_size);
	parse_if (buf, "date",  props->show_date);
	parse_if (buf, "user",  props->show_user);
	parse_if (buf, "group", props->show_group);

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
	cherokee_list_content_free (&props->notice_files, free);

	cherokee_buffer_mrproper (&props->header);
	cherokee_buffer_mrproper (&props->footer);
	cherokee_buffer_mrproper (&props->entry);
	cherokee_buffer_mrproper (&props->css);

	return cherokee_handler_props_free_base (HANDLER_PROPS(props));
}


ret_t 
cherokee_handler_dirlist_configure (cherokee_config_node_t *conf, cherokee_server_t *srv, cherokee_module_props_t **_props)
{
	ret_t                             ret;
	cherokee_list_t                  *i;
	cherokee_handler_dirlist_props_t *props;
	char                             *theme      = NULL;
	cherokee_buffer_t                 theme_path = CHEROKEE_BUF_INIT;

	if (*_props == NULL) {
		CHEROKEE_NEW_STRUCT (n, handler_dirlist_props);
		
		cherokee_handler_props_init_base (HANDLER_PROPS(n), 
						  MODULE_PROPS_FREE(cherokee_handler_dirlist_props_free));

		n->show_size   = true;
		n->show_date   = true;
		n->show_user   = false;
		n->show_group  = false;

		cherokee_buffer_init (&n->header);
		cherokee_buffer_init (&n->footer);
		cherokee_buffer_init (&n->entry);
		cherokee_buffer_init (&n->css);

		INIT_LIST_HEAD (&n->notice_files);
		
		*_props = MODULE_PROPS(n);
	}
	
	props = PROP_DIRLIST(*_props);

	cherokee_config_node_foreach (i, conf) {
		cherokee_config_node_t *subconf = CONFIG_NODE(i);

		/* Convert the properties
		 */
		if (equal_buf_str (&subconf->key, "size")) {
			props->show_size = atoi (subconf->val.buf);
		} else if (equal_buf_str (&subconf->key, "date")) {
			props->show_date = atoi (subconf->val.buf);
		} else if (equal_buf_str (&subconf->key, "user")) {
			props->show_user = atoi (subconf->val.buf);
		} else if (equal_buf_str (&subconf->key, "group")) {
			props->show_group = atoi (subconf->val.buf);

		} else if (equal_buf_str (&subconf->key, "theme")) {
			theme = subconf->val.buf;
		
		} else if (equal_buf_str (&subconf->key, "notice_files")) {
			ret = cherokee_config_node_convert_list (subconf, NULL, &props->notice_files);
			if (unlikely (ret != ret_ok)) return ret;
		}
	}

	/* Load the theme
	 */
	if (theme == NULL)
		theme = "default";

	ret = cherokee_buffer_add_va (&theme_path, CHEROKEE_THEMEDIR"/%s/", theme);
	if (ret != ret_ok) return ret;
	
	ret = load_theme (&theme_path, props);
	if (ret != ret_ok) {
		PRINT_MSG ("Couldn't load theme '%s': %s\n", theme, theme_path.buf);
		return ret;
	}
	
	cherokee_buffer_mrproper (&theme_path);
	return ret_ok;
}


static cherokee_boolean_t
is_header_file (cherokee_handler_dirlist_t *dhdl, char *filename)
{
	cherokee_list_t *i;

	list_for_each (i, &HDL_DIRLIST_PROP(dhdl)->notice_files) 
	{
		if (strcmp (filename, LIST_ITEM_INFO(i)) == 0)
			return true;
	}

	return false;
}


ret_t
generate_file_entry (cherokee_handler_dirlist_t *dhdl, DIR *dir, cherokee_buffer_t *path, file_entry_t **ret_entry)
{
	int            re;
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
	n = (file_entry_t *) malloc (sizeof(file_entry_t) + path->len + extra + 3);
	if (unlikely(n == NULL)) return ret_nomem;

	INIT_LIST_HEAD(&n->list_entry);

	for (;;) {
		/* Read a new directory entry
		 */
		cherokee_readdir (dir, &n->info, &entry);
		if (entry == NULL) {
			free (n);
			return ret_eof;
		}

		name = (char *)entry->d_name;
		n->name_len = strlen(name);

		/* Skip some files
		 */
		if ((name[0] == '.') ||
		    (name[0] == '#') ||
		    (name[n->name_len-1] == '~') ||
		    is_header_file (dhdl, name))
		{
			continue;
		}
		
		/* Build the local path, stat and clean
		 */
		cherokee_buffer_add (path, name, n->name_len);
		
		/* Check the filename size
		 */
		if (n->name_len > dhdl->longest_filename)
			dhdl->longest_filename = n->name_len;
		
		/* Path	
		 */
		re = cherokee_stat (path->buf, &n->stat);
		if (re < 0) {
			cherokee_buffer_drop_endding (path, n->name_len);

			free (n);
			return ret_error;
		}

		/* Clean up and exit
		 */
		cherokee_buffer_drop_endding (path, n->name_len);

		*ret_entry = n;
		return ret_ok;
	}

	return ret_eof;
}


ret_t
cherokee_handler_dirlist_new  (cherokee_handler_t **hdl, void *cnt, cherokee_module_props_t *props)
{	
	ret_t  ret;
	char  *value;
	CHEROKEE_NEW_STRUCT (n, handler_dirlist);
	
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
	n->software_str_ref = NULL;
	n->longest_filename = 0;

	/* Choose the sorting key
	 */
	n->phase = dirlist_phase_add_header;
	n->sort  = Name_Down;

	ret = cherokee_table_get (HANDLER_CONN(n)->arguments, "order", (void **) &value);
	if (ret == ret_ok) {
		if      (value[0] == 'N') n->sort = Name_Up;
		else if (value[0] == 'n') n->sort = Name_Down;
		else if (value[0] == 'D') n->sort = Date_Up;
		else if (value[0] == 'd') n->sort = Date_Down;
		else if (value[0] == 'S') n->sort = Size_Up;
		else if (value[0] == 's') n->sort = Size_Down;
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
		PRINT_ERROR_S ("The theme is incomplete\n");
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
		free (i);
	}

	list_for_each_safe (i, tmp, &dhdl->files) {
		cherokee_list_del (i);
		free (i);
	}

	return ret_ok;
}


static ret_t
check_request_finish_with_slash (cherokee_handler_dirlist_t *dhdl)
{
	cherokee_connection_t *conn = HANDLER_CONN(dhdl);

	if ((cherokee_buffer_is_empty (&conn->request)) ||
	    (!cherokee_buffer_is_endding (&conn->request, '/')))
	{
		cherokee_buffer_clean (&conn->redirect);
		cherokee_buffer_ensure_size (&conn->redirect, conn->request.len + conn->userdir.len + 4);

		if (! cherokee_buffer_is_empty (&conn->userdir)) {
			cherokee_buffer_add_str (&conn->redirect, "/~");
			cherokee_buffer_add_buffer (&conn->redirect, &conn->userdir);
		}

		cherokee_buffer_add_buffer (&conn->redirect, &conn->request);
		cherokee_buffer_add_str (&conn->redirect, "/");
		
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

	return strcmp((char*) &f1->info.d_name, (char*) &f2->info.d_name);
}


static int 
cmp_size_down (cherokee_list_t *a, cherokee_list_t *b)
{
	int           diff;
	file_entry_t *f1 = (file_entry_t *)a;
	file_entry_t *f2 = (file_entry_t *)b;
	
	diff = f1->stat.st_size - f2->stat.st_size;
	if (diff == 0) cmp_name_down (a,b);

	return diff;
}

static int 
cmp_date_down (cherokee_list_t *a, cherokee_list_t *b)
{
	int           diff;
	file_entry_t *f1 = (file_entry_t *)a;
	file_entry_t *f2 = (file_entry_t *)b;

	diff = f1->stat.st_mtime - f2->stat.st_mtime;
	if (diff == 0) cmp_name_down (a,b);

	return diff;
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
#ifndef CHEROKEE_EMBEDDED
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
#endif
}


static ret_t
build_file_list (cherokee_handler_dirlist_t *dhdl)
{
	DIR                   *dir;
	file_entry_t          *item;
	cherokee_connection_t *conn  = HANDLER_CONN(dhdl);

	/* Build the local directory path
	 */
	cherokee_buffer_add_buffer (&conn->local_directory, &conn->request);        /* 1 */
	dir = opendir (conn->local_directory.buf);

	if (dir == NULL) {
		conn->error_code = http_not_found;
		return ret_error;
	}

	/* Read the files
	 */
	for (;;) {
		ret_t ret;
		
		ret = generate_file_entry (dhdl, dir, &conn->local_directory, &item);
		if (ret == ret_eof) 
			break;
		if ((ret == ret_nomem) ||
		    (ret == ret_error))
			continue;

		if (S_ISDIR(item->stat.st_mode)) {
			cherokee_list_add (LIST(item), &dhdl->dirs);
		} else {
			cherokee_list_add (LIST(item), &dhdl->files);
		}
	}

	/* Clean
	 */
	closedir(dir);
	cherokee_buffer_drop_endding (&conn->local_directory, conn->request.len); /* 2 */

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
		cuint_t  filename_len;
		char    *filename = LIST_ITEM_INFO(i);

		filename_len = strlen(filename);
		cherokee_buffer_clean (&dhdl->header);
	        
		if (filename[0] != '/') {
			cherokee_buffer_add_buffer (&conn->local_directory, &conn->request);                     /* do   */
			cherokee_buffer_add (&conn->local_directory, filename, filename_len);
			
			ret = cherokee_buffer_read_file (&dhdl->header, conn->local_directory.buf);
			cherokee_buffer_drop_endding (&conn->local_directory, conn->request.len + filename_len); /* undo */
		} else {
			ret = cherokee_buffer_read_file (&dhdl->header, filename);
		}

		if (ret == ret_ok)
			break;
	}

	return ret_ok;
}


ret_t
cherokee_handler_dirlist_init (cherokee_handler_dirlist_t *dhdl)
{
	ret_t                  ret;
	cherokee_connection_t *conn = HANDLER_CONN(dhdl);
	cherokee_server_t     *srv  = HANDLER_SRV(dhdl);

	/* The request must end with a slash..
	 */
	ret = check_request_finish_with_slash (dhdl);
	if (ret != ret_ok) return ret;
	
	if (! cherokee_list_empty (&HDL_DIRLIST_PROP(dhdl)->notice_files)) {
		ret = read_notice_file (dhdl);
		if (ret != ret_ok) return ret;
	}

	/* Build de local request
	 */
	ret = build_file_list (dhdl);
	if (unlikely(ret < ret_ok)) return ret;
	
	/* Build public dir string
	 */
	ret = build_public_path (dhdl, &dhdl->public_dir);
	if (unlikely (ret != ret_ok)) return ret;
	
	/* Server software string
	 */
	if (conn->socket.is_tls == non_TLS)
		dhdl->software_str_ref = &srv->ext_server_w_port_string;
	else
		dhdl->software_str_ref = &srv->ext_server_w_port_tls_string;

 	return ret_ok;
}


static ret_t
replace_token_guts (cherokee_buffer_t *buf, char *token, char token_len, char *replacement)
{
	if (replacement == NULL) {
		return cherokee_buffer_replace_string (buf, token, token_len, "", 0);
	}

	return cherokee_buffer_replace_string (buf, token, token_len, replacement, strlen(replacement));
}

static ret_t
render_file (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer, file_entry_t *file)
{
	cherokee_boolean_t                is_dir;
	char                             *alt      = NULL;
	char                             *icon     = NULL;
	char                             *name     = (char *) &file->info.d_name;
	cherokee_icons_t                 *icons    = HANDLER_SRV(dhdl)->icons;
	cherokee_buffer_t                *tmp      = &dhdl->header;
	cherokee_handler_dirlist_props_t *props    = HDL_DIRLIST_PROP(dhdl);

#define replace_token(buf,token,val) \
	replace_token_guts(buf,token,sizeof(token)-1,val)

	/* Add entry text
	 */
	cherokee_buffer_add_buffer (buffer, &props->entry);
	
	/* Add the icon
	 */
	is_dir = S_ISDIR(file->stat.st_mode);

	alt = (is_dir) ? "[DIR]" : "[   ]";
	
#ifndef CHEROKEE_EMBEDDED
	if (icons != NULL) {
		if (is_dir) {
			icon = icons->directory_icon.buf;
		} else {
			cherokee_icons_get_icon (icons, name, &icon);
			if (icon == NULL) icon = "blank.png";
		}
	}
#endif

	replace_token (buffer, "%icon_alt%", alt);
	replace_token (buffer, "%icon%", icon);

	/* File
	 */
	replace_token (buffer, "%file_name%", name);
	replace_token (buffer, "%file_link%", name);

	/* Date
	 */
	if (props->show_date) {
		cherokee_buffer_clean (tmp);
		cherokee_buffer_ensure_size (tmp, 33);

		strftime (tmp->buf, 32, "%d-%b-%Y %H:%M", localtime(&file->stat.st_mtime));
		replace_token (buffer, "%date%", tmp->buf);
	} 

	/* Size
	 */
	if (props->show_size) {
		if (is_dir) {
			replace_token (buffer, "%size_unit%", NULL);
			replace_token (buffer, "%size%", "-");
		} else {
			char *unit;

			cherokee_buffer_clean (tmp);
			cherokee_buffer_ensure_size (tmp, 6);

			cherokee_strfsize (file->stat.st_size, tmp->buf);

			unit = tmp->buf;
			while ((*unit >= '0')  && (*unit <= '9')) unit++;

			replace_token (buffer, "%size_unit%", unit);
			*unit = '\0';
			replace_token (buffer, "%size%", tmp->buf);
		}
	} 

	/* User
	 */
	if (props->show_user) {
		struct passwd *user;
		char          *name;
		
		user = getpwuid (file->stat.st_uid);
		name = (char *) (user->pw_name) ? user->pw_name : "unknown";

		replace_token (buffer, "%user%", name);
	} 

	/* Group
	 */
	if (props->show_group) {
		struct group *user;
		char         *group;
		
		user = getgrgid (file->stat.st_gid);
		group = (char *) (user->gr_name) ? user->gr_name : "unknown";
		
		replace_token (buffer, "%group%", group);
	}

	return ret_ok;
}


static ret_t
render_parent_directory (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer)
{
	char                             *icon     = NULL;
	cherokee_icons_t                 *icons    = HANDLER_SRV(dhdl)->icons;
	cherokee_handler_dirlist_props_t *props    = HDL_DIRLIST_PROP(dhdl);

	cherokee_buffer_add_buffer (buffer, &props->entry);

#ifndef CHEROKEE_EMBEDDED
	if (icons != NULL) {
		icon = icons->parentdir_icon.buf;
	}
#endif

	replace_token (buffer, "%icon%", icon);
	replace_token (buffer, "%icon_alt%", "[DIR]");

	replace_token (buffer, "%file_link%", "../");
	replace_token (buffer, "%file_name%", "Parent Directory");

	replace_token (buffer, "%date%", NULL);
	replace_token (buffer, "%size_unit%", NULL);
	replace_token (buffer, "%size%", "-");
	replace_token (buffer, "%user%", NULL);
	replace_token (buffer, "%group%", NULL);

	return ret_ok;
}


static ret_t
replace_header_footer_vbles (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer)
{
	/* Public dir
	 */
	replace_token (buffer, "%public_dir%", dhdl->public_dir.buf);

	/* Server software
	 */
	replace_token (buffer, "%server_software%", dhdl->software_str_ref->buf);

	/* Notice
	 */
	replace_token (buffer, "%notice%", dhdl->header.buf);

	/* Orders
	 */
	replace_token (buffer, "%order_name%", 
		       (dhdl->sort == Name_Down) ? "N" : "n");
	replace_token (buffer, "%order_size%", 
		       (dhdl->sort == Size_Down) ? "S" : "s");
	replace_token (buffer, "%order_date%", 
		       (dhdl->sort == Date_Down) ? "D" : "d");

	return ret_ok;
}


ret_t
cherokee_handler_dirlist_step (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer)
{
	ret_t                             ret;
	cherokee_handler_dirlist_props_t *props = HDL_DIRLIST_PROP(dhdl);

	switch (dhdl->phase) {
	case dirlist_phase_add_header:
		/* Add the theme header
		 */
		cherokee_buffer_add_buffer (buffer, &props->header);

		ret = replace_header_footer_vbles (dhdl, buffer);
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

			/* Maybe it has readed enought
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

			/* Maybe it has readed enought
			 */
			if (buffer->len > DEFAULT_READ_SIZE) 
				return ret_ok;
		}

		dhdl->phase = dirlist_phase_add_footer;

	case dirlist_phase_add_footer:
		cherokee_buffer_add_buffer (buffer, &props->footer);

		ret = replace_header_footer_vbles (dhdl, buffer);
		if (unlikely (ret != ret_ok)) return ret;

		break;
	}

	return ret_eof_have_data;
}


ret_t
cherokee_handler_dirlist_add_headers (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer)
{
	cherokee_buffer_add_str (buffer, "Content-Type: text/html; charset=iso-8859-1"CRLF);
	return ret_ok;
}
