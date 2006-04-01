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

#include "icons_grammar.h"

#include <stdio.h>
#include <string.h>
%}

%option yylineno
%option nounput

ws              [ \t]+
comment         #.*$
fpath           \/[^\"\{\}\n ]*
id              [\.\-_a-zA-Z0-9]+
id_wildcats     [\.\-_a-zA-Z0-9\*\?]+
nl              [\n\r]+

%%

{nl}          { return T_NEWLINE; }
{ws}          ;
{comment}     ;

"File"            { return T_FILE; }
"Suffix"          { return T_SUFFIX; }
"Default"         { return T_DEFAULT; }
"Directory"       { return T_DIRECTORY; }
"ParentDirectory" { return T_PARENT; }

{id}          { yy_icons_lval.string = strdup (yy_icons_text); return T_ID; }
{id_wildcats} { yy_icons_lval.string = strdup (yy_icons_text); return T_ID_WILDCATS; }
{fpath}       { yy_icons_lval.string = strdup (yy_icons_text); return T_FULLDIR; }

<<EOF>>       { yyterminate(); }

.		    { return yy_icons_text[0]; }


%%

int 
yy_icons_wrap()
{
       return 1;
}

