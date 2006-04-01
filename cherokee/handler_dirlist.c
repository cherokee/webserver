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
#include "list_merge_sort.h"
#include "util.h"
#include "connection.h"
#include "connection-protected.h"
#include "server.h"
#include "server-protected.h"
#include "module_loader.h"
#include "icons.h"
#include "common.h"
#include "list_ext.h"

#define DEFAULT_NAME_LEN 40


cherokee_module_info_handler_t MODULE_INFO(dirlist) = {
	.module.type     = cherokee_handler,               /* type         */
	.module.new_func = cherokee_handler_dirlist_new,   /* new func     */
	.valid_methods   = http_get                        /* http methods */
};


struct file_entry {
	struct list_head list_entry;
	struct stat      stat;
	cuint_t          name_len;
	struct dirent    info;          /* It *must* be the last entry */
};
typedef struct file_entry file_entry_t;


ret_t
generate_file_entry (DIR *dir, cherokee_buffer_t *path, cherokee_handler_dirlist_t *dhdl, file_entry_t **ret_entry)
{
	int            re;
	file_entry_t  *n;
	char          *name;
	cuint_t        extra;
	struct dirent *entry;
	
	/* Get the memory
	 */
#ifdef _PC_NAME_MAX
	extra = pathconf(path->buf, _PC_NAME_MAX);
#else
	extra = PATH_MAX;
#endif
	n = (file_entry_t *) malloc (sizeof(file_entry_t) + path->len + extra + 3);
	if (unlikely(n == NULL)) return ret_nomem;

	/* Read a new directory entry
	 */
	cherokee_readdir (dir, &n->info, &entry);
	if (entry == NULL) return ret_eof;

	/* Initialization
	 */
	INIT_LIST_HEAD(&n->list_entry);

	/* Build the local path, stat and clean
	 */
	name = (char *)entry->d_name;
	n->name_len = strlen(name);
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


ret_t
cherokee_handler_dirlist_new  (cherokee_handler_t **hdl, void *cnt, cherokee_table_t *properties)
{	
	ret_t  ret;
	char  *value;
	CHEROKEE_NEW_STRUCT (n, handler_dirlist);
	
	/* Init the base class object
	 */
	cherokee_handler_init_base (HANDLER(n), cnt);

	MODULE(n)->init         = (handler_func_init_t) cherokee_handler_dirlist_init;
	MODULE(n)->free         = (module_func_free_t) cherokee_handler_dirlist_free;
	MODULE(n)->get_name     = (module_func_get_name_t) cherokee_handler_dirlist_get_name;
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

	/* Choose the sorting key
	 */
	n->sort = Name_Down;
	ret = cherokee_table_get (HANDLER_CONN(n)->arguments, "order", (void **) &value);
	if (ret == ret_ok) {
		if      (value[0] == 'N') n->sort = Name_Up;
		else if (value[0] == 'n') n->sort = Name_Down;
		else if (value[0] == 'D') n->sort = Date_Up;
		else if (value[0] == 'd') n->sort = Date_Down;
		else if (value[0] == 'S') n->sort = Size_Up;
		else if (value[0] == 's') n->sort = Size_Down;
	}

	/* State
	 */
	n->page_header      = false;
	n->dir_ptr          = NULL;
	n->file_ptr         = NULL;
	n->longest_filename = 0;

	/* Properties
	 */
	n->show_size        = 1;
	n->show_date        = 1;
	n->show_owner       = 0;
	n->show_group       = 0;
	
	/* Eyecandy
	 */
	n->bgcolor          = "FFFFFF";
	n->text             = "000000";
	n->link             = "0000AA";
	n->vlink            = "0000CC";
	n->alink            = "0022EE";
	n->background       = NULL;
	n->header_file      = NULL;
	n->header_file_ref  = NULL;
 	n->build_headers    = true;

	cherokee_buffer_init (&n->header);

	/* Read the properties
	 */
	if (properties) {
		cherokee_typed_table_get_str (properties, "bgcolor", &n->bgcolor);
		cherokee_typed_table_get_str (properties, "text", &n->text);
		cherokee_typed_table_get_str (properties, "link", &n->link);
		cherokee_typed_table_get_str (properties, "vlink", &n->vlink);
		cherokee_typed_table_get_str (properties, "alink", &n->alink);
		cherokee_typed_table_get_str (properties, "background", &n->background);

		cherokee_typed_table_get_int (properties, "size", &n->show_size);
		cherokee_typed_table_get_int (properties, "date", &n->show_date);
		cherokee_typed_table_get_int (properties, "owner", &n->show_owner);
		cherokee_typed_table_get_int (properties, "group", &n->show_group);

		cherokee_typed_table_get_list (properties, "headerfile", &n->header_file);

		cherokee_typed_table_get_int (properties, "show_headerfile", &n->build_headers);
	}

	*hdl = HANDLER(n);
	return ret_ok;
}


ret_t
cherokee_handler_dirlist_free (cherokee_handler_dirlist_t *dhdl)
{
	list_t *i, *tmp;

	cherokee_buffer_mrproper (&dhdl->header);

	list_for_each_safe (i, tmp, &dhdl->dirs) {
		list_del (i);
		free (i);
	}

	list_for_each_safe (i, tmp, &dhdl->files) {
		list_del (i);
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

static void
read_header_file (cherokee_handler_dirlist_t *dhdl)
{
	ret_t                  ret;
	list_t                *i;
	cherokee_connection_t *conn = HANDLER_CONN(dhdl);

	list_for_each (i, dhdl->header_file) {
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

		if (ret == ret_ok) {
			dhdl->header_file_ref = filename;
			return;
		}
	}
}


static int 
cmp_name_down (struct list_head *a, struct list_head *b)
{
	file_entry_t *f1 = (file_entry_t *)a;
	file_entry_t *f2 = (file_entry_t *)b;

	return strcmp((char*) &f1->info.d_name, (char*) &f2->info.d_name);
}


static int 
cmp_size_down (struct list_head *a, struct list_head *b)
{
	int           diff;
	file_entry_t *f1 = (file_entry_t *)a;
	file_entry_t *f2 = (file_entry_t *)b;
	
	diff = f1->stat.st_size - f2->stat.st_size;
	if (diff == 0) cmp_name_down (a,b);

	return diff;
}

static int 
cmp_date_down (struct list_head *a, struct list_head *b)
{
	int           diff;
	file_entry_t *f1 = (file_entry_t *)a;
	file_entry_t *f2 = (file_entry_t *)b;

	diff = f1->stat.st_mtime - f2->stat.st_mtime;
	if (diff == 0) cmp_name_down (a,b);

	return diff;
}

static int 
cmp_name_up (struct list_head *a, struct list_head *b)
{
	return -cmp_name_down(a,b);
}

static int 
cmp_size_up (struct list_head *a, struct list_head *b)
{
	return -cmp_size_down(a,b);
}

static int 
cmp_date_up (struct list_head *a, struct list_head *b)
{
	return -cmp_date_down(a,b);
}

static void
list_sort_by_type (struct list_head *list, cherokee_sort_t sort)
{
	switch (sort) {
	case Name_Down:
		list_sort (list, cmp_name_down);
		break;
	case Name_Up:
		list_sort (list, cmp_name_up);
		break;
	case Size_Down:
		list_sort (list, cmp_size_down);
		break;
	case Size_Up:
		list_sort (list, cmp_size_up);
		break;
	case Date_Down:
		list_sort (list, cmp_date_down);
		break;
	case Date_Up:
		list_sort (list, cmp_date_up);
		break;
	}
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
		
		ret = generate_file_entry (dir, &conn->local_directory, dhdl, &item);
		if (ret == ret_eof) break;
		if (ret == ret_error) continue;

		if (S_ISDIR(item->stat.st_mode)) {
			list_add ((list_t *)item, &dhdl->dirs);
		} else {
			list_add ((list_t *)item, &dhdl->files);
		}
	}

	/* Clean
	 */
	closedir(dir);
	cherokee_buffer_drop_endding (&conn->local_directory, conn->request.len); /* 2 */

	/* Sort the file list
	 */
	if (! list_empty(&dhdl->files)) {
		list_sort_by_type (&dhdl->files, dhdl->sort);
		dhdl->file_ptr = dhdl->files.next;
	}

	/* Sort the directories list:
	 * it doesn't make sense to look for the size
	 */
	if (! list_empty(&dhdl->dirs)) {
		cherokee_sort_t sort = dhdl->sort;

		if (sort == Size_Down) sort = Name_Down;
		if (sort == Size_Up)   sort = Name_Up;

		list_sort_by_type (&dhdl->dirs, sort);
		dhdl->dir_ptr = dhdl->dirs.next;
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
	if (ret != ret_ok) return ret;

	/* Maybe read the header file
	 */
	if (dhdl->header_file != NULL) {
		read_header_file (dhdl);
	}

	/* Build de local request
	 */
	ret = build_file_list (dhdl);
	if (unlikely(ret < ret_ok)) return ret;
	
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
render_page_header (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer)
{
	cherokee_connection_t *conn;
 	cherokee_icons_t      *icons;
	cherokee_buffer_t      path = CHEROKEE_BUF_INIT;
	
//	CHEROKEE_NEW(path,buffer);

	conn  = HANDLER_CONN(dhdl);
	icons = HANDLER_SRV(dhdl)->icons;
	
	if (dhdl->build_headers) {
		cherokee_buffer_add_str (buffer, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">" CRLF);
	
		/* Step 1:
		 * Build the public path
		 */
		build_public_path (dhdl, &path);
	
		/* Add some HTML: title and body begin
		 */
		cherokee_buffer_add_str (buffer, "<html><head><title>Index of ");
		cherokee_buffer_add_buffer (buffer, &path);
		cherokee_buffer_add_str (buffer, "</title></head><body");
	
		cherokee_buffer_add_va (buffer, " bgcolor=\"%s\" text=\"%s\" link=\"%s\" vlink=\"%s\" alink=\"%s\"", 
					dhdl->bgcolor, dhdl->text, dhdl->link, dhdl->vlink, dhdl->alink);
	
		if (dhdl->background) {
			cherokee_buffer_add_va (buffer, " background=\"%s\"", dhdl->background);
		}
	
		cherokee_buffer_add_str (buffer, "><h1>Index of ");
		cherokee_buffer_add_buffer (buffer, &path);
		cherokee_buffer_add_str (buffer, "</h1>");

	} else {
		/* The custom header should be enough
		 */
		cherokee_buffer_add_buffer (buffer, &dhdl->header);
		cherokee_buffer_mrproper (&dhdl->header);
	}

	cherokee_buffer_add_str (buffer, "<pre>");
	
	/* Step 2:
	 * Print the ordering bar
	 */
#ifndef CHEROKEE_EMBEDDED
	if (icons && (icons->parentdir_icon != NULL))
		cherokee_buffer_add_str (buffer, "<img src=\"/icons/blank.png\">");
	else
#endif	
		cherokee_buffer_add_str (buffer, "   ");
	
	cherokee_buffer_add_va (buffer, " <a href=\"?order=%c\">Name</a>",
				(dhdl->sort == Name_Down) ? 'N' : 'n');

	cherokee_buffer_add_char_n (buffer, ' ', MAX(dhdl->longest_filename, DEFAULT_NAME_LEN) - 3);
	
	if (dhdl->show_date) {
		cherokee_buffer_add_va (buffer, "<a href=\"?order=%c\">Last Modification</a>   ", 
					(dhdl->sort == Date_Down) ? 'D' : 'd');
	}
	
	if (dhdl->show_size) {
		cherokee_buffer_add_va (buffer, "<a href=\"?order=%c\">Size</a>         ",
					(dhdl->sort == Size_Down) ? 'S' : 's');
	}
	
	cherokee_buffer_add_str (buffer, "<hr>");
	
	/* Step 3:
	 * Free the path, we will not need it again
	 */
	cherokee_buffer_mrproper (&path);
	
	
	if (! cherokee_buffer_is_empty (&dhdl->header)) {
		cherokee_buffer_add_buffer (buffer, &dhdl->header);
	}
	
#ifndef CHEROKEE_EMBEDDED
	if (icons && (icons->parentdir_icon != NULL)) {
		cherokee_buffer_add_va (buffer, "<img border=\"0\" src=\"/icons/%s\" alt=\"[DIR]\"> <a href=\"..\">Parent Directory</a>\n", 
					icons->parentdir_icon);
	} else
#endif	
		cherokee_buffer_add_str (buffer, "<a href=\"..\">Parent Directory</a>\n");
	
	return ret_ok;
}


static ret_t
render_file (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer, file_entry_t *file)
{
	int               is_dir;
	char             *icon;
	cherokee_icons_t *icons    = HANDLER_SRV(dhdl)->icons;
	char             *name     = (char *) &file->info.d_name;
	cuint_t           name_len = file->name_len;

	/* Ignore backup files
	 */
	if ((name[0] == '.') ||
	    (name[0] == '#') ||
	    (name[name_len-1] == '~') ||
	    ((dhdl->header_file_ref) && (strcmp (name, dhdl->header_file_ref) == 0)))
	{
		return ret_ok;
	}
	
	/* Add the icon
	 */
	icon   = "";
	is_dir = S_ISDIR(file->stat.st_mode);

#ifndef CHEROKEE_EMBEDDED
	if (icons != NULL) {
		if (is_dir && (icons->directory_icon != NULL)) {
			cherokee_buffer_add_va (buffer, "<img border=\"0\" src=\"/icons/%s\" alt=\"[DIR]\"> ", icons->directory_icon);
		} else {
			cherokee_icons_get_icon (icons, name, &icon);
			cherokee_buffer_add_va (buffer, "<img border=\"0\" src=\"/icons/%s\" alt=\"[   ]\"> ", icon);
		}
	} else
#endif	
		cherokee_buffer_add (buffer, (char *) (is_dir) ? "[DIR] " : "[   ] ", 6);
	
	/* Add the filename
	 */
	if (is_dir) {
		cherokee_buffer_add_va (buffer, "<a href=\"%s/\">%s/</a>", name, name); 
		name_len++;
	} else {
		cherokee_buffer_add_va (buffer, "<a href=\"%s\">%s</a>", name, name); 
	}
	
	
	/* Maybe add more info
	 */
	if (dhdl->show_size  || dhdl->show_date  ||
	    dhdl->show_owner || dhdl->show_group) 
	{
		int spaces;

		spaces = MAX(DEFAULT_NAME_LEN, dhdl->longest_filename) - name_len;
		cherokee_buffer_add_char_n (buffer, ' ', spaces+1);

			
		if (dhdl->show_date) {				
			int len;
			char tmp[32];
			len = strftime (tmp, 32, "%d-%b-%Y %H:%M   ", localtime(&file->stat.st_mtime));
			cherokee_buffer_add (buffer, tmp, len);
		}

		if (dhdl->show_size) {
			char tmp[5];			

			if (S_ISDIR(file->stat.st_mode)) {
				memcpy (tmp, "   -\0", 5);
			} else {
				cherokee_strfsize (file->stat.st_size, tmp);
			}
			cherokee_buffer_add_va (buffer, "%s  ", tmp);
		}

		if (dhdl->show_owner) {
			struct passwd *user;
			char          *name;

			user = getpwuid (file->stat.st_uid);
			name = (char *) (user->pw_name) ? user->pw_name : "unknown";

			cherokee_buffer_add_va (buffer, "%s", name);
		}

		if (dhdl->show_group) {
			struct group *user;
			char         *group;

			user = getgrgid (file->stat.st_gid);
			group = (char *) (user->gr_name) ? user->gr_name : "unknown";

			cherokee_buffer_add_va (buffer, "%s", group);
		}
	}
		
	cherokee_buffer_add_str (buffer, "\n");
	return ret_ok;
}



ret_t
cherokee_handler_dirlist_step (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer)
{
	cuint_t                port;
	cherokee_connection_t *conn = HANDLER_CONN(dhdl);

	/* Page header
	 */
	if (dhdl->page_header == false) {
		render_page_header (dhdl, buffer);
		dhdl->page_header = true;
	}

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

	/* Page ending
	 */
	cherokee_buffer_add_str (buffer, "</pre><hr>\n");

	if (HANDLER_CONN(dhdl)->socket.is_tls == non_TLS)
 		port = HANDLER_SRV(dhdl)->port;
	else 
 		port = HANDLER_SRV(dhdl)->port_tls;

	if (CONN_SRV(conn)->server_token == cherokee_version_full) 
		cherokee_buffer_add_version (buffer, port, ver_full_html);
	else 
		cherokee_buffer_add_version (buffer, port, ver_port_html);

	cherokee_buffer_add_str (buffer, "\n</body></html>");

	return ret_eof_have_data;
}


ret_t
cherokee_handler_dirlist_add_headers (cherokee_handler_dirlist_t *dhdl, cherokee_buffer_t *buffer)
{
	cherokee_buffer_add_str (buffer, "Content-Type: text/html"CRLF);
	return ret_ok;
}


void  
cherokee_handler_dirlist_get_name (cherokee_handler_dirlist_t *dhdl, char **name)
{
	*name = "dirlist";
}


/* Library init function
 */
void
MODULE_INIT(dirlist) (cherokee_module_loader_t *loader)
{
}
