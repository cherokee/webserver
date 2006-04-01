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

#include "read_config_grammar.h"

#include <stdio.h>
#include <string.h>
%}

%option yylineno
%option nounput

ws              [ \t]+
integer         [-+]?[0-9]+
comment         #.*$
fpath_win       ([a-zA-Z]:[^\"\n\r]*)
fpath           (\/[^\"\n\r ]*)|{fpath_win}
qstring         \"[^\"\{\}\n\r]*\"
http            http\:\/{fpath}
https           https\:\/{fpath}
id              [_\-\.a-zA-Z][\.\-_a-zA-Z0-9]*
ip4_end         [0-9]*[0-9]+
ip4_part        [0-2]*{ip4_end}
ip4_dir         {ip4_part}\.{ip4_part}\.{ip4_part}\.{ip4_part}
ip4_mask        [\/\\]({ip4_dir}|[0-9]{1,2})
ip4             {ip4_dir}{ip4_mask}{0,1}
domain_name     [\.\-_a-zA-Z0-9]*
mime_type       [a-z0-9\-]+\/[a-z0-9\-]+

hex_255         [0-9a-fA-F]{1,4}
ip6_part        {hex_255}":"?
ip6_part2       {hex_255}[\:\.]?
ip6_form1       {ip6_part}{0,16}"::"{ip6_part2}{0,16}
ip6_form2       ({hex_255}":"){16}{hex_255}
ip6_addr        {ip6_form1}|{ip6_form2}
ip6_mask        [\/\\]("1"?[0-9]{1,2}|{ip6_addr})
ip6             {ip6_addr}{ip6_mask}?

ip              {ip4}|{ip6}
address_port    ({domain_name}|{ip})":"{integer}
nl              [\n\r]

%%

"Handler"               { return T_HANDLER; }
"Port"                  { return T_PORT; }
"PortTLS"               { return T_PORT_TLS; }
"DocumentRoot"          { return T_DOCUMENT_ROOT; }
"LogFlushInterval"      { return T_LOG_FLUSH_INTERVAL; }
"Log"                   { return T_LOG; }
"Timeout"               { return T_TIMEOUT; }
"MimeFile"              { return T_MIME_FILE; }
"Directory"             { return T_DIRECTORY; }
"Extension"             { return T_EXTENSION; }
"Request"               { return T_REQUEST; }
"MimeType"              { return T_MIMETYPE; }
"KeepAlive"             { return T_KEEPALIVE; }
"MaxKeepAliveRequests"  { return T_KEEPALIVE_MAX_REQUESTS; }
"ErrorHandler"          { return T_ERROR_HANDLER; }
"User"                  { return T_USER; }
"Group"                 { return T_GROUP; }
"Server"                { return T_SERVER; }
"Userdir"               { return T_USERDIR; }
"PidFile"               { return T_PIDFILE; }
"Listen"                { return T_LISTEN; }
"Chroot"                { return T_CHROOT; }
"PanicAction"           { return T_PANIC_ACTION; }
"ServerTokens"          { return T_SERVER_TOKENS; }
"Encoder"               { return T_ENCODER; }
"ThreadNumber"          { return T_THREAD_NUM; }
"DirectoryIndex"        { return T_DIRECTORYINDEX; }
"IPv6"                  { return T_IPV6; }
"Icons"                 { return T_ICONS; }
"Auth"                  { return T_AUTH; }
"Name"                  { return T_NAME; }
"Method"                { return T_METHOD; }
"PasswdFile"            { return T_PASSWDFILE; }
"Show"                  { return T_SHOW; }
"Rewrite"               { return T_REWRITE; }
"Allow"                 { return T_ALLOW; }
"Deny"                  { return T_DENY; }
"From"                  { return T_FROM; }
"socket"                { return T_SOCKET;}
"justabout"             { return T_JUST_ABOUT; }
"SSLCertificateFile"    { return T_SSL_CERT_FILE; }
"SSLCertificateKeyFile" { return T_SSL_CERT_KEY_FILE; }
"SSLCAListFile"         { return T_SSL_CA_LIST_FILE; }
"Sendfile"              { return T_SENDFILE; }
"MinSize"               { return T_MINSIZE; }
"MaxSize"               { return T_MAXSIZE; }
"ListenQueueSize"       { return T_LISTEN_QUEUE_SIZE; }
"MaxFds"                { return T_MAX_FDS; }
"PollMethod"            { return T_POLL_METHOD; }
"policy"                { return T_POLICY; }
"OnlySecure"            { return T_ONLY_SECURE; }
"MaxConnectionReuse"    { return T_MAX_CONNECTION_REUSE; }
"MaxAge"                { return T_MAX_AGE; }
"Env"                   { return T_ENV; }
"HeaderFile"            { return T_HEADERFILE; }
"On"                    { yylval.number = 1; return T_NUMBER; }
"Off"                   { yylval.number = 0; return T_NUMBER; }

{fpath}          { yylval.string = strdup(yytext); return T_FULLDIR; }
{http}           { yylval.string = strdup(yytext); return T_HTTP_URL; }
{https}          { yylval.string = strdup(yytext); return T_HTTPS_URL; }
{id}             { yylval.string = strdup(yytext); return T_ID; }
{mime_type}      { yylval.string = strdup(yytext); return T_MIMETYPE_ID; }
{ip}             { yylval.string = strdup(yytext); return T_IP; }
{qstring}        { int len = strlen(yytext);
                   yylval.string = (char *) malloc (len-1);
                   memcpy (yylval.string, yytext+1, len-2);
                   yylval.string[len-2]='\0';
                   return T_QSTRING; 
                 }

{integer}        { yylval.number = atoi(yytext); return T_NUMBER; }
{domain_name}    { yylval.string = strdup(yytext); return T_DOMAIN_NAME; }
{address_port}   { yylval.string = strdup(yytext); return T_ADDRESS_PORT; }

{comment}        ;
{nl}             ;
{ws}             ;

<<EOF>>          { yyterminate(); }

.		       { return yytext[0]; }

%%

int 
yywrap()
{
       return 1;
}

