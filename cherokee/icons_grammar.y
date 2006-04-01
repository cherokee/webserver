%{
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
#include "list_ext.h"
#include "icons.h"

/* Define the parameter name of the yyparse() argument
 */
#define YYPARSE_PARAM icons

%}

%union {
	   char             *string;
	   struct list_head *list;
}

%{
/* What is the right way to import these prototipes?
 * There's the same problem in read_config_grammar.y
 */
extern int   yy_icons_lex(void);
extern char *yy_icons_text;
extern int   yy_icons_lineno;

static list_t  current_list;


void
yy_icons_error(char* msg)
{
        PRINT_ERROR ("ERROR: parsing configuration: '%s', line %d, symbol '%s' (0x%x)\n", 
				 msg, yy_icons_lineno, yy_icons_text, yy_icons_text[0]);
}

%}

%token T_DEFAULT T_DIRECTORY T_PARENT T_SUFFIX T_FILE T_NEWLINE 
%token <string> T_FULLDIR T_ID T_ID_WILDCATS

%type <list> list_ids
%type <string> id

%%

icons_file : lines;

lines : line
      | lines line;

line : suffixes
     | files
     | default
     | directory
     | parent
     | nl
     ; 

nl: T_NEWLINE;
maybe_nl : | nl;

id : T_ID
   | T_ID_WILDCATS;

/* Files
 */
files : T_FILE maybe_nl '{' maybe_nl files_entries maybe_nl '}';

files_entries : files_entries file_entry
              | file_entry;

file_entry : T_ID list_ids
{
	   cherokee_icons_set_files (ICONS(icons), $2, $1);
	   cherokee_list_free ($2, free);
	   free ($1);
}


/* Suffixes
 */
suffixes : T_SUFFIX maybe_nl '{' maybe_nl suffixes_entries maybe_nl '}';

suffixes_entries : suffix_entry
                 | suffixes_entries suffix_entry;

suffix_entry : T_ID list_ids
{
	   cherokee_icons_set_suffixes (ICONS(icons), $2, $1);	   
	   cherokee_list_free ($2, free);
	   free ($1);
}


/* Default
 */
default : T_DEFAULT T_ID
{
	   cherokee_icons_set_default (ICONS(icons), $2);
};

/* Directory
 */
directory : T_DIRECTORY T_ID
{
	   cherokee_icons_set_directory (ICONS(icons), $2);
};

/* Parent directory
 */
parent : T_PARENT T_ID
{
	   cherokee_icons_set_parentdir (ICONS(icons), $2);
};


list_ids : id T_NEWLINE
{
	   INIT_LIST_HEAD(&current_list);
	   cherokee_list_add (&current_list, $1);
	   $$ = &current_list;
};

list_ids : id ',' list_ids
{
	   cherokee_list_add (&current_list, $1);
	   $$ = &current_list;
};
