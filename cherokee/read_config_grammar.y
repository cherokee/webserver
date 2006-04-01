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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <strings.h>

#ifdef HAVE_PWD_H
# include <pwd.h>
#endif

#ifdef HAVE_GRP_H
# include <grp.h>
#endif

#include "table.h"
#include "mime.h"
#include "server.h"
#include "server-protected.h"
#include "virtual_server.h"
#include "config_entry.h"
#include "encoder.h"
#include "logger_table.h"
#include "access.h"
#include "list.h"
#include "list_ext.h"
#include "reqs_list.h"
#include "reqs_list_entry.h"
#include "ext_source.h"


/* Define the parameter name of the yyparse() argument
 */
#define YYPARSE_PARAM server

%}


%union {
	   int   number;
	   char *string;
	   void *ptr;

	   struct {
			 char *name;
			 void *ptr;
	   } name_ptr;

	   void *list;
}


%{
/* What is the right way to import this prototipe?
 */
extern int yylex (void);


extern char *yytext;
extern int   yylineno;

char                                   *current_yacc_file        = NULL;
static cherokee_dirs_table_t           *current_dirs_table       = NULL;
static cherokee_config_entry_t         *current_config_entry     = NULL;
static list_t                          *current_reqs_list        = NULL;
static cherokee_virtual_server_t       *current_virtual_server   = NULL;
static cherokee_encoder_table_entry_t  *current_encoder_entry    = NULL;
static cherokee_module_info_t          *current_module_info      = NULL;
static cherokee_ext_source_t           *current_ext_source       = NULL;
static cherokee_mime_entry_t           *current_mime_entry       = NULL;
static cuint_t                          priority_counter         = CHEROKEE_CONFIG_PRIORITY_DEFAULT + 1;

typedef struct {
	   void *next;
	   char *string;
} linked_list_t;

struct {
	   char                           *handler_name;
	   cherokee_config_entry_t        *entry;
	   cherokee_virtual_server_t      *vserver;
	   cherokee_dirs_table_t          *dirs;
	   char                           *document_root;
	   char                           *directory_name;
} directory_content_tmp;

struct {
	   char                           *handler_name;
	   cherokee_config_entry_t        *entry;
	   cherokee_virtual_server_t      *vserver;
	   cherokee_exts_table_t          *exts;
	   char                           *document_root;
	   linked_list_t                  *exts_list;
} extension_content_tmp;

struct {
	   char                           *handler_name;
	   cherokee_reqs_list_entry_t     *entry;
	   cherokee_virtual_server_t      *vserver;
	   list_t                         *reqs;
	   char                           *document_root;
} request_content_tmp;


#define auto_virtual_server ((current_virtual_server) ? current_virtual_server : SRV(server)->vserver_default)
#define auto_dirs_table     ((current_dirs_table) ? current_dirs_table : &(auto_virtual_server)->dirs)
#define auto_reqs_table     ((current_reqs_list) ? current_reqs_list : &(auto_virtual_server)->reqs)

static void
free_linked_list (linked_list_t *list, void (*free_func) (void *))
{
	   linked_list_t *i = list;

	   while (i != NULL) {
			 linked_list_t *prev = i;

			 if ((free_func) && (i->string)) {
				    free_func (i->string);
			 }

			 prev = i;
			 i = i->next;
			 free (prev);
	   }	   
}

static char *
remove_last_slash (char *str)
{
	   cuint_t len = strlen(str);
	   
	   if (len <= 2)
			 return str;

	   if (str[len-1] == '/') {
			 str[len-1] = '\0';
	   }

	   return str;
}

static char *
make_finish_with_slash (char *string, int *len)
{
	   char *new_entry;
	   int   new_len;

	   if (string[(*len)-1] == '/') {
			 return string;
	   }

	   new_len = (*len) + 2;
	   new_entry  = (char*) malloc (new_len);

	   *len = snprintf (new_entry, new_len, "%s/", string);

	   free (string);
	   return new_entry;
}

static char *
make_slash_end (char *string)
{
	   int len = strlen(string);
	   return make_finish_with_slash (string, &len);
}

static cherokee_config_entry_t *
config_entry_new (void)
{
	   cherokee_config_entry_t *entry;

	   cherokee_config_entry_new (&entry);
	   current_config_entry = entry;

	   /* Assign the priority
	    */
	   entry->priority = priority_counter;
	   priority_counter++;

	   return entry;
}

static cherokee_reqs_list_entry_t *
reqs_list_entry_new (void)
{
	   cherokee_reqs_list_entry_t *entry;

	   cherokee_reqs_list_entry_new (&entry);
	   current_config_entry = (cherokee_config_entry_t *)entry;      /* It is okay! */

	   entry->base_entry.priority = priority_counter;
	   priority_counter++;

	   return entry;
}

static char *
new_string_to_lowercase (const char *in)
{
	   int   i;
	   char *tmp;
	   
	   i = strlen(in);
	   tmp = (char *) malloc (i+1);
	   tmp[i] = '\0';

	   while (i--) {
			 tmp[i] = tolower(in[i]);
	   }

	   return tmp;
}

static int
load_module (cherokee_module_loader_t *loader, char *name, cherokee_module_info_t **info)
{
	   ret_t ret;

	   ret = cherokee_module_loader_load (loader, name);
	   if (ret < ret_ok) {
			 PRINT_MSG("ERROR: Loading module '%s'\n", name);
			 return 1;
	   }
	   
	   ret = cherokee_module_loader_get_info (loader, name, info);
	   if (ret < ret_ok) {
			 PRINT_MSG("ERROR: Loading module '%s'\n", name);
			 return 1;
	   }

	   return 0;
}


static void
handler_redir_add_property (cherokee_config_entry_t *entry, char *regex, char *subs, int show)
{
	   char   *p;
	   char   *serialized;
	   int     regex_len    = 0;
	   int     subs_len     = 0;
	   list_t *plist        = NULL;
	   list_t  nlist        = LIST_HEAD_INIT(nlist);

	   /* Build the string:
	    * [1]show [s]regex \0 [s]subs \0
	    */
	   subs_len = strlen(subs);

	   if (regex != NULL)
			 regex_len = strlen(regex);

	   serialized = (char *) malloc (1 + regex_len + 1 + subs_len + 1);
	   memset (serialized, 0, regex_len + subs_len + 3);

	   p = serialized;

	   *p = show;
	   p++;

	   strncpy (p, regex, regex_len);
	   p += regex_len + 1;
	   
	   strncpy (p, subs, subs_len);
	   
	   /* Add it to the list
	    */
	   if (entry->handler_properties != NULL) {
			 cherokee_typed_table_get_list (entry->handler_properties, "regex_list", &plist);
	   }

	   if (plist == NULL) {
			 cherokee_list_add (&nlist, serialized);
			 cherokee_config_entry_set_handler_prop (entry, "regex_list", typed_list, &nlist, 
												(cherokee_typed_free_func_t) cherokee_list_free_item_simple);
	   } else {
			 cherokee_list_add_tail (plist, serialized);
	   }
}

static void
handler_redir_add_property_simple (cherokee_config_entry_t *entry, char *subs, int show)
{
	   handler_redir_add_property (entry, NULL, subs, show);
}


static void
dirs_table_set_handler_prop (cherokee_config_entry_t *dir_entry, char *prop, char *value)
{
	   cherokee_config_entry_set_handler_prop (dir_entry, prop, typed_str, value, NULL);
}

static void
dirs_table_set_validator_prop (cherokee_config_entry_t *dir_entry, char *prop, char *value)
{
	   cherokee_config_entry_set_validator_prop (dir_entry, prop, typed_str, value, NULL);
}

static int
add_key_val_entry_in_property (cherokee_table_t *properties, char *prop_name, char *key, char *val)
{
	   cuint_t           new_len;
	   char             *new_str;
	   list_t           *plist       = NULL;
	   list_t            nlist       = LIST_HEAD_INIT(nlist);
	   
	   /* Build the string:
	    * VAR \0 VAL \0
	    */
	   new_len = strlen(key) + strlen(val) + 2;
	   new_str = malloc (new_len);
	   if (new_str == NULL) return 1;
	   
	   memset (new_str, 0, new_len);
	   memcpy (new_str, key, strlen(key));
	   memcpy (new_str + strlen(key) + 1, val, strlen(val));

	   /* Add it to the list
	    */
	   if (properties != NULL) {
			 cherokee_typed_table_get_list (properties, prop_name, &plist);
	   }
	   
	   if (plist == NULL) {
			 cherokee_list_add (&nlist, new_str);
			 cherokee_config_entry_set_handler_prop (current_config_entry, prop_name, typed_list, &nlist, 
											 (cherokee_typed_free_func_t) cherokee_list_free_item_simple);
	   } else {
			 cherokee_list_add_tail (plist, new_str);			 
	   }

	   return 0;
}

static ret_t
split_address_or_path (char *str, cherokee_buffer_t *hostname, cint_t *port_num, 
				   cherokee_buffer_t *unix_socket, cherokee_buffer_t *original)
{
	   char    *p;
	   cuint_t  len;

	   len = strlen(str);
	   if (len <= 0) return ret_error;

	   /* Original
	    */
	   cherokee_buffer_add (original, str, len);

	   /* Unix socket
	    */
	   if (str[0] == '/') {
			 cherokee_buffer_add (unix_socket, str, len);
			 return ret_ok;
	   } 

	   /* Host name
	    */
	   p = strchr(str, ':');
	   if (p == NULL) {
			 cherokee_buffer_add (hostname, str, len);
			 return ret_ok;
	   } 

	   /* Host name + port
	    */
	   *p = '\0';
	   *port_num = atoi (p+1);
	   cherokee_buffer_add (hostname, str, p - str);
	   *p = ':';

	   return ret_ok;
}

static char *
fix_win32_path (char *str)
{
#ifdef _WIN32
	   char *i = str;

	   /* Replace '\' by '/'
	    */
	   while (*i) {
			 if (*i == '\\') *i='/';
			 i++;
	   }

	   /* Remove the spaces at the end of the string
	    */
	   i--;
	   while ((i > str) && (*i == ' ')) {
			 *i='\0'; 
			 i--; 			 
	   }
#endif 
	   return str;
}


void
yyerror (char* msg)
{
	   char *config;

	   config = (current_yacc_file) ? current_yacc_file : "";

        PRINT_MSG("Error parsing file %s:%d '%s', symbol '%s'\n", 
			   config, yylineno, msg, yytext);
}

%}


%token T_QUOTE T_DENY T_THREAD_NUM T_SSL_CERT_KEY_FILE T_SSL_CERT_FILE T_KEEPALIVE_MAX_REQUESTS T_ERROR_HANDLER
%token T_TIMEOUT T_KEEPALIVE T_DOCUMENT_ROOT T_LOG T_MIME_FILE T_DIRECTORY T_HANDLER T_USER T_GROUP T_POLICY
%token T_SERVER T_USERDIR T_PIDFILE T_LISTEN T_SERVER_TOKENS T_ENCODER T_ALLOW T_DIRECTORYINDEX 
%token T_ICONS T_AUTH T_NAME T_METHOD T_PASSWDFILE T_SSL_CA_LIST_FILE T_FROM T_SOCKET T_LOG_FLUSH_INTERVAL
%token T_HEADERFILE T_PANIC_ACTION T_JUST_ABOUT T_LISTEN_QUEUE_SIZE T_SENDFILE T_MINSIZE T_MAXSIZE T_MAX_FDS
%token T_SHOW T_CHROOT T_ONLY_SECURE T_MAX_CONNECTION_REUSE T_REWRITE T_POLL_METHOD T_EXTENSION T_IPV6 T_ENV 
%token T_REQUEST T_MIMETYPE T_MAX_AGE

%token <number> T_NUMBER T_PORT T_PORT_TLS
%token <string> T_QSTRING T_FULLDIR T_ID T_HTTP_URL T_HTTPS_URL T_HOSTNAME T_IP T_DOMAIN_NAME T_ADDRESS_PORT T_MIMETYPE_ID

%type <name_ptr> directory_option handler
%type <string> host_name http_generic id_or_path ip_or_domain str_type address_or_path fulldir
%type <list> id_list ip_list domain_list id_path_list

%%


conffile : /* Empty */
         | lines
         ; 

lines : line
	 | lines line
	 ;

server_lines : server_line
             | server_lines server_line
             ;

line : server_line
     | common_line
     ;

common_line : server
            | port
            | port_tls
            | listen
            | server_tokens
            | mime
            | mime_entry
            | icons
            | timeout
            | keepalive
            | keepalive_max_requests
            | pidfile
            | user1 
            | user2
            | group1 
            | group2
            | chroot
            | thread_number
            | ipv6
            | log_flush_interval
            | poll_method
            | panic_action
            | listen_queue_size
            | sendfile
            | maxfds
            | maxconnectionreuse
            ;

server_line : directory
            | extension
            | request
            | errorhandler
            | document_root
            | directoryindex
            | log
            | encoder
            | ssl_file
            | ssl_key_file
            | ssl_ca_list_file
            | userdir
            ;

handler_server_optinal_entries:
                              | handler_server_optinal_entries handler_server_optinal_entry;

directory_options : 
                  | directory_options directory_option;

sendfile_options : 
                 | sendfile_options sendfile_option;

handler_options : 
                | handler_options handler_option;

encoder_options :
                | encoder_options encoder_option;

thread_options : 
               | thread_options thread_option;

auth_options :
             | auth_options auth_option;

directories : 
            | directories directory;


/* Path fix up
 */
fulldir : T_FULLDIR
{
	   $$ = fix_win32_path($1);
};

/* ID/Path List
 */
id_or_path : T_QSTRING { $$ = $1; }
           | T_ID      { $$ = $1; }
           | fulldir   { $$ = $1; };

id_path_list : id_or_path
{
	   linked_list_t *n = (linked_list_t *) malloc(sizeof(linked_list_t));
	   n->next = NULL;
	   n->string = $1;
	   
	   $$ = n;
};

id_path_list : id_or_path ',' id_path_list
{
	   linked_list_t *n = (linked_list_t *) malloc(sizeof(linked_list_t));
	   n->next = $3;
	   n->string = $1;

	   $$ = n;
};


/* ID List
 */
id_list : T_ID 
{
	   linked_list_t *n = (linked_list_t *) malloc(sizeof(linked_list_t));
	   n->next = NULL;
	   n->string = $1;
	   
	   $$ = n;
};

id_list : T_ID ',' id_list
{
	   linked_list_t *n = (linked_list_t *) malloc(sizeof(linked_list_t));
	   n->next = $3;
	   n->string = $1;

	   $$ = n;
};


/* Domain names list
 */
domain_list : host_name
{
	   linked_list_t *n = (linked_list_t *) malloc(sizeof(linked_list_t));
	   n->next = NULL;
	   n->string = $1;
	   
	   $$ = n;
};

domain_list : host_name ',' domain_list
{
	   linked_list_t *n = (linked_list_t *) malloc(sizeof(linked_list_t));
	   n->next = $3;
	   n->string = $1;

	   $$ = n;
};


/* IP List
 */
ip_or_domain : T_IP | T_ID;

ip_list : ip_or_domain
{
	   linked_list_t *n = (linked_list_t *) malloc (sizeof(linked_list_t));
	   n->next   = NULL;
	   n->string = $1;

	   $$ = n;
};

ip_list : ip_or_domain ',' ip_list
{
	   linked_list_t *n = (linked_list_t *) malloc (sizeof(linked_list_t));
	   n->next   = $3;
	   n->string = $1;

	   $$ = n;
};

port : T_PORT T_NUMBER 
{
	   SRV(server)->port = $2;
};

port_tls : T_PORT_TLS T_NUMBER 
{
	   SRV(server)->port_tls = $2;
};

listen : T_LISTEN host_name
{
	   SRV(server)->listen_to = $2;
};

log_flush_interval : T_LOG_FLUSH_INTERVAL T_NUMBER
{
	   SRV(server)->log_flush_elapse = $2;
};

poll_method : T_POLL_METHOD T_ID
{
	   if (strcmp($2, "epoll") == 0) {
			 SRV(server)->fdpoll_method = cherokee_poll_epoll;
	   } else if (strcmp($2, "port") == 0) {
			 SRV(server)->fdpoll_method = cherokee_poll_port;
	   } else if (strcmp($2, "kqueue") == 0) {
			 SRV(server)->fdpoll_method = cherokee_poll_kqueue;
	   } else if (strcmp($2, "poll") == 0) {
			 SRV(server)->fdpoll_method = cherokee_poll_poll;
	   } else if (strcmp($2, "win32") == 0) {
			 SRV(server)->fdpoll_method = cherokee_poll_win32;
	   } else if (strcmp($2, "select") == 0) {
			 SRV(server)->fdpoll_method = cherokee_poll_select;
	   } else {
			 PRINT_MSG ("ERROR: Unknown polling method '%s'\n", $2);
			 return 1;
	   }
};

document_root : T_DOCUMENT_ROOT id_or_path
{
	   char                      *root;
	   int                        root_len;
	   cherokee_virtual_server_t *vserver;

	   vserver = auto_virtual_server;

	   /* It might need to fix the path
	    */
	   fix_win32_path($2);

	   root     = $2;
	   root_len = strlen($2);

	   /* Check for the endding slash
	    */
	   root = make_finish_with_slash (root, &root_len);

	   /* Add the virtual root path to the virtual server struct
	    */
	   cherokee_buffer_add (vserver->root, root, root_len);
};


log : T_LOG T_ID
{
	   ret_t ret;

	   /* Maybe load the module
	    */
	   ret = cherokee_module_loader_load (&SRV(server)->loader, $2);
	   if (ret < ret_ok) {
			 PRINT_MSG ("ERROR: Can't load logger module '%s'\n", $2);
			 return 1;
	   }

	   cherokee_module_loader_get_info (&SRV(server)->loader, $2, &current_module_info);
}
log_optional
{
	   cherokee_virtual_server_t *vserver;
	   vserver = auto_virtual_server;

	   /* Instance the logger object
	    */
	   cherokee_logger_table_new_logger (SRV(server)->loggers, $2, current_module_info,
								  vserver->logger_props, &vserver->logger);
	   current_module_info = NULL;
};

log_optional : 
             | '{' tuple_list '}';

tuple_list : 
           | tuple_list tuple
           ;

tuple : T_ID fulldir
{
	   cherokee_virtual_server_t *vserver;
	   vserver = auto_virtual_server;

	   if (vserver->logger_props == NULL) {
			 cherokee_table_new (&vserver->logger_props);
	   }

	   cherokee_typed_table_add_str (vserver->logger_props, $1, $2);
};


server_tokens : T_SERVER_TOKENS T_ID
{
	   if (!strncasecmp("Product", $2, 7)) {
			 SRV(server)->server_token = cherokee_version_product;
	   } else if (!strncasecmp("Minor", $2, 5)) {
			 SRV(server)->server_token = cherokee_version_minor;
	   } else if (!strncasecmp("Minimal", $2, 7)) {
			 SRV(server)->server_token = cherokee_version_minimal;
	   } else if (!strncasecmp("OS", $2, 2)) {
			 SRV(server)->server_token = cherokee_version_os;
	   } else if (!strncasecmp("Full", $2, 4)) {
			 SRV(server)->server_token = cherokee_version_full;
	   } else {
			 PRINT_MSG ("ERROR: Unknown server token '%s'\n", $2);
			 return 1;
	   }
};

mime : T_MIME_FILE fulldir
{
	   SRV(server)->mime_file = $2;
};

mime_entry : T_MIMETYPE T_MIMETYPE_ID '{'
{
	   ret_t                  ret;
	   cherokee_mime_entry_t *entry;

	   if (SRV(server)->mime == NULL) {
			 ret = cherokee_mime_new (&SRV(server)->mime);
			 if (ret != ret_ok) return 1;
	   }

	   ret = cherokee_mime_get_by_type (SRV(server)->mime, $2, &entry);
	   if (ret != ret_ok) {
			 cherokee_mime_entry_new (&entry);
			 cherokee_mime_add_entry (SRV(server)->mime, entry);
	   }

	   current_mime_entry = entry;
}
mimetype_options '}'
{
	   current_mime_entry = NULL;
}

mimetype_options : T_MAX_AGE T_NUMBER
{
	   cherokee_mime_entry_set_maxage (current_mime_entry, $2);
};

mimetype_options : T_EXTENSION id_list
{
	   ret_t          ret;
	   linked_list_t *i;

	   i = $2;
	   while (i != NULL) {			 
			 ret = cherokee_mime_set_by_suffix (SRV(server)->mime, i->string, current_mime_entry);
			 if (ret != ret_ok) return ret;

			 free (i->string);
			 i = i->next;
	   }
};

icons : T_ICONS fulldir
{
	   SRV(server)->icons_file = $2;
};

timeout : T_TIMEOUT T_NUMBER
{
	   SRV(server)->timeout = $2;

	   cherokee_buffer_clean  (SRV(server)->timeout_header);
	   cherokee_buffer_add_va (SRV(server)->timeout_header, "Keep-Alive: timeout=%d"CRLF, $2);
};

keepalive : T_KEEPALIVE T_NUMBER
{
	   SRV(server)->keepalive = ($2 == 0) ? false : true;
};

keepalive_max_requests : T_KEEPALIVE_MAX_REQUESTS T_NUMBER
{
	   SRV(server)->keepalive_max = $2;
};

ssl_file : T_SSL_CERT_FILE fulldir
{
#ifdef HAVE_TLS
	   cherokee_virtual_server_t *vsrv = auto_virtual_server;

	   if (vsrv->server_cert != NULL) {
			 PRINT_MSG ("ERROR: \"SSLCertificateFile\" overlaps: '%s' <- '%s'\n", vsrv->server_cert, $2);
			 free (vsrv->server_cert);
	   }

	   vsrv->server_cert = $2;

#else
	   PRINT_MSG_S ("WARNING: Ignoring SSL configuration entry: \"SSLCertificateFile\"\n");
#endif
};

ssl_key_file : T_SSL_CERT_KEY_FILE fulldir
{
#ifdef HAVE_TLS
	   cherokee_virtual_server_t *vsrv = auto_virtual_server;

	   if (vsrv->server_key != NULL) {
			 PRINT_MSG ("ERROR: \"SSLCertificateKeyFile\" overlaps: '%s' <- '%s'\n", vsrv->server_key, $2);
			 free (vsrv->server_key);
	   }

	   vsrv->server_key = $2;

#else
	   PRINT_MSG_S ("WARNING: Ignoring SSL configuration entry: \"SSLCertificateKeyFile\"\n");
#endif
};

ssl_ca_list_file : T_SSL_CA_LIST_FILE fulldir
{
#ifdef HAVE_TLS
	   cherokee_virtual_server_t *vsrv = auto_virtual_server;

	   if (vsrv->ca_cert != NULL) {
			 PRINT_MSG ("ERROR: \"SSLCAListFile\" overlaps: '%s' <- '%s'\n", vsrv->ca_cert, $2);
			 free (vsrv->ca_cert);
	   }

	   vsrv->ca_cert = $2;

#else
	   PRINT_MSG_S ("WARNING: Ignoring SSL configuration entry: \"SSLCAListFile\"\n");
#endif
};


encoder : T_ENCODER T_ID
{
	   ret_t ret;
	   cherokee_module_info_t *info;
	   cherokee_encoder_table_entry_t *enc;

	   /* Load the module
	    */
	   ret = cherokee_module_loader_load (&SRV(server)->loader, $2);
	   if (ret < ret_ok) {
			 PRINT_MSG ("ERROR: Can't load encoder module '%s'\n", $2);
			 return 1;
	   }

	   cherokee_module_loader_get_info  (&SRV(server)->loader, $2, &info);

	   /* Set the info in the new entry
	    */
	   cherokee_encoder_table_entry_new (&enc);
	   cherokee_encoder_table_entry_get_info (enc, info);

	   /* Set in the encoders table
	    */
	   cherokee_encoder_table_set (SRV(server)->encoders, $2, enc);
	   current_encoder_entry = enc;
} 
maybe_encoder_options
{
	   current_encoder_entry = NULL;
};

maybe_encoder_options : '{' encoder_options '}' 
                      |;

encoder_option : T_ALLOW id_list
{
	   linked_list_t            *i;
	   cherokee_matching_list_t *matching;

	   cherokee_matching_list_new (&matching);
	   cherokee_encoder_entry_set_matching_list (current_encoder_entry, matching);

	   i = $2;
	   while (i!=NULL) {
			 linked_list_t *prev;

			 cherokee_matching_list_add_allow (matching, i->string);

			 free(i->string);
			 prev = i;
			 i = i->next;
			 free(prev);
	   }
};

encoder_option : T_DENY id_list
{
	   linked_list_t            *i;
	   cherokee_matching_list_t *matching;

	   cherokee_matching_list_new (&matching);
	   cherokee_encoder_entry_set_matching_list (current_encoder_entry, matching);

	   i = $2;
	   while (i!=NULL) {
			 linked_list_t *prev;

			 cherokee_matching_list_add_deny (matching, i->string);

			 free(i->string);
			 prev = i;
			 i = i->next;
			 free(prev);
	   }
};

pidfile : T_PIDFILE fulldir
{
	   cherokee_buffer_clean (&SRV(server)->pidfile);
	   cherokee_buffer_add (&SRV(server)->pidfile, $2, strlen($2));

	   free ($2);
};

panic_action : T_PANIC_ACTION fulldir
{
	   if (SRV(server)->panic_action != NULL) {
			 PRINT_MSG ("WARNING: Overwriting panic action '%s' by '%s'\n", SRV(server)->panic_action, $2);
			 free (SRV(server)->panic_action);
	   }

	   SRV(server)->panic_action = $2;
};

listen_queue_size : T_LISTEN_QUEUE_SIZE T_NUMBER
{
	   SRV(server)->listen_queue = $2;
};

sendfile : T_SENDFILE '{' sendfile_options '}';

sendfile_option : T_MINSIZE T_NUMBER 
{
	   SRV(server)->sendfile.min = $2;
};

sendfile_option : T_MAXSIZE T_NUMBER 
{
	   SRV(server)->sendfile.max = $2;
};

maxfds : T_MAX_FDS T_NUMBER
{
	   SRV(server)->max_fds = $2;
};

maxconnectionreuse : T_MAX_CONNECTION_REUSE T_NUMBER
{
	   SRV(server)->max_conn_reuse = $2;
};

chroot : T_CHROOT fulldir
{
	   SRV(server)->chroot = $2;
};

thread_number : T_THREAD_NUM T_NUMBER maybe_thread_options
{
#ifdef HAVE_PTHREAD
	   SRV(server)->thread_num = $2;
#endif
};

maybe_thread_options : '{' thread_options '}'
                     |;

thread_option : T_POLICY T_ID
{
#ifdef HAVE_PTHREAD
	   if (strcasecmp($2, "fifo") == 0) {
			 SRV(server)->thread_policy = SCHED_FIFO;
	   } else if (strcasecmp($2, "rr") == 0) {
			 SRV(server)->thread_policy = SCHED_RR;
	   } else if (strcasecmp($2, "other") == 0) {
			 SRV(server)->thread_policy = SCHED_OTHER;
	   } else {
			 PRINT_MSG ("ERROR: unknown scheduling policy '%s'\n", $2);
	   }
#endif
};


ipv6 : T_IPV6 T_NUMBER
{
	   SRV(server)->ipv6 = $2;
};

user1 : T_USER T_ID
{
	   struct passwd *pwd;
	   
	   pwd = (struct passwd *) getpwnam ($2);
	   if (pwd == NULL) {
			 PRINT_MSG ("ERROR: User '%s' not found in the system", $2);
			 return 1;
	   }

	   SRV(server)->user = pwd->pw_uid;

	   free ($2);
};

user2 : T_USER T_NUMBER
{
	   SRV(server)->user = $2;
};

group1 : T_GROUP T_ID
{
	   struct group *grp;

	   grp = (struct group *) getgrnam ($2);
	   if (grp == NULL) {
			 PRINT_MSG ("ERROR: Group '%s' not found in the system", $2);
			 return 1;
	   }

	   SRV(server)->group = grp->gr_gid;

	   free ($2);
};

group2 : T_GROUP T_NUMBER
{
	   SRV(server)->group = $2;
};

handler : T_HANDLER T_ID  '{' handler_options '}' 
{
	   $$.name = $2;
	   $$.ptr = current_config_entry;
};

handler : T_HANDLER T_ID 
{
	   $$.name = $2;
	   $$.ptr = current_config_entry;
};

http_generic : T_HTTP_URL  { $$ = $1; }
             | T_HTTPS_URL { $$ = $1; };



handler_option : T_SHOW T_REWRITE T_QSTRING T_QSTRING
{
	   handler_redir_add_property (current_config_entry, $3, $4, 1);
}

handler_option : T_REWRITE T_QSTRING T_QSTRING
{
	   handler_redir_add_property (current_config_entry, $2, $3, 0);
};

handler_option : T_SHOW T_REWRITE T_QSTRING 
{
	   handler_redir_add_property_simple (current_config_entry, $3, 1);
}

handler_option : T_REWRITE T_QSTRING
{
	   handler_redir_add_property_simple (current_config_entry, $2, 0);
};

handler_option : T_KEEPALIVE T_NUMBER
{
	   cherokee_config_entry_set_handler_prop (current_config_entry, "nkeepalive", typed_int, INT_TO_POINTER($2), NULL);	   
};

handler_option : T_SOCKET T_NUMBER
{
	   cherokee_config_entry_set_handler_prop (current_config_entry, "nsocket", typed_int, INT_TO_POINTER($2), NULL);	   
};

str_type : T_ID
         | fulldir
         | T_ADDRESS_PORT
         | T_QSTRING
         | T_HTTP_URL
         | T_HTTPS_URL
{ 
	   $$ = $1; 
};

handler_option : T_ID str_type
{
	   if (!strcasecmp ($1, "bgcolor")) {
			 dirs_table_set_handler_prop (current_config_entry, "bgcolor", $2);
	   } else if (!strcasecmp ($1, "background")) {
			 dirs_table_set_handler_prop (current_config_entry, "background", $2);
	   } else if (!strcasecmp ($1, "text")) {
			 dirs_table_set_handler_prop (current_config_entry, "text", $2);
	   } else if (!strcasecmp ($1, "link")) {
			 dirs_table_set_handler_prop (current_config_entry, "link", $2);
	   } else if (!strcasecmp ($1, "vlink")) {
			 dirs_table_set_handler_prop (current_config_entry, "vlink", $2);
	   } else if (!strcasecmp ($1, "alink")) {
			 dirs_table_set_handler_prop (current_config_entry, "alink", $2);
	   } else if (!strcasecmp ($1, "interpreter")) {
			 dirs_table_set_handler_prop (current_config_entry, "interpreter", fix_win32_path($2));
	   } else if (!strcasecmp ($1, "scriptalias")) {
			 dirs_table_set_handler_prop (current_config_entry, "scriptalias", fix_win32_path($2));
	   } else if (!strcasecmp ($1, "url")) {
			 dirs_table_set_handler_prop (current_config_entry, "url", $2);
	   } else if (!strcasecmp ($1, "filedir")) {
			 dirs_table_set_handler_prop (current_config_entry, "filedir", fix_win32_path($2));
	   } else if (!strcasecmp ($1, "changeuser")) {
			 cherokee_config_entry_set_handler_prop (current_config_entry, "changeuser", typed_int, INT_TO_POINTER($2), NULL);
	   } else {
			 return 1;
	   }
};


handler_option : T_ID T_NUMBER
{
	   if (!strcasecmp ($1, "changeuser")) {
			 cherokee_config_entry_set_handler_prop (current_config_entry, "changeuser", typed_int, INT_TO_POINTER($2), NULL);
	   } else if (!strcasecmp ($1, "iocache")) {
			 cherokee_config_entry_set_handler_prop (current_config_entry, "cache", typed_int, INT_TO_POINTER($2), NULL);
	   } else if (!strcasecmp ($1, "checkfile")) {
			 cherokee_config_entry_set_handler_prop (current_config_entry, "checkfile", typed_int, INT_TO_POINTER($2), NULL);
	   } else {
			 return 1;
	   } 
};

handler_option : T_ERROR_HANDLER T_NUMBER
{
	   cherokee_config_entry_set_handler_prop (current_config_entry, "errorhandler", typed_int, INT_TO_POINTER($2), NULL);
};

handler_option : T_SHOW T_HEADERFILE T_NUMBER
{
	   cherokee_config_entry_set_handler_prop (current_config_entry, "show_headerfile", typed_int, INT_TO_POINTER($3), NULL);
};


handler_option : T_HEADERFILE id_path_list
{
	   linked_list_t *i;
	   list_t         nlist = LIST_HEAD_INIT(nlist);

	   i = $2;
	   while (i!=NULL) {
			 linked_list_t *prev;

			 cherokee_list_add_tail (&nlist, i->string);

			 prev = i;
			 i = i->next;
			 free(prev);
	   }	   

	   cherokee_config_entry_set_handler_prop (current_config_entry, "headerfile", typed_list, &nlist, 
									   (cherokee_typed_free_func_t) cherokee_list_free_item_simple);
};

handler_option : T_ENV T_ID str_type
{
	   int re;

	   re = add_key_val_entry_in_property (current_config_entry->handler_properties, "env", $2, $3);
	   if (re != 0) return re;
};

handler_option : T_SOCKET fulldir
{ dirs_table_set_handler_prop (current_config_entry, "socket", $2); };

handler_option : T_JUST_ABOUT
{ cherokee_config_entry_set_handler_prop (current_config_entry, "about", typed_int, INT_TO_POINTER(1), NULL); };

handler_option : T_SERVER address_or_path 
{ 
	   ret_t                  ret;
	   list_t                 nlist        = LIST_HEAD_INIT(nlist);
	   list_t                *plist        = NULL;
	   cherokee_table_t      *properties;
	   cherokee_ext_source_t *server_entry = NULL;
	   
	   /* Add the new entry to the list
	    */
	   properties = current_config_entry->handler_properties;

	   if (properties != NULL) {
			 cherokee_typed_table_get_list (properties, "servers", &plist);
	   }

	   /* The list is new
	    */
	   if (plist == NULL) {
			 cherokee_ext_source_head_t *head = NULL;
			 
			 ret = cherokee_ext_source_head_new (&head);
			 if (ret != ret_ok) return 1;

			 server_entry = EXT_SOURCE(head);

			 list_add ((list_t *)server_entry, &nlist);
			 cherokee_config_entry_set_handler_prop (current_config_entry, "servers", typed_list, &nlist, 
											 (cherokee_typed_free_func_t) cherokee_ext_source_free);
	   } 
	   /* Add to an existing list
	    */
	   else {
			 cherokee_ext_source_new (&server_entry);
			 list_add_tail ((list_t *)server_entry, plist);
	   }

	   current_ext_source = server_entry;
	   split_address_or_path ($2, &server_entry->host, &server_entry->port,
						 &server_entry->unix_socket, &server_entry->original_server);

} handler_server_optinal;


address_or_path : T_ADDRESS_PORT { $$ = $1; }
                | fulldir        { $$ = $1; };

handler_server_optinal : 
                       | '{' handler_server_optinal_entries '}';

handler_server_optinal_entry : T_ENV T_ID str_type 
{
	   ret_t ret;

	   ret = cherokee_ext_source_add_env (current_ext_source, $2, $3);
	   if (ret != ret_ok) return 1;
};

handler_server_optinal_entry: T_ID str_type
{
	   if (strcasecmp($1, "interpreter") == 0) {
			 fix_win32_path($2);
			 cherokee_buffer_add (&current_ext_source->interpreter, $2, strlen($2));
	   } else {
			 return 1;
	   }
};

handler_option : T_NUMBER http_generic
{
	   char code[4];

	   if (($1 < 100) || ($1 >= http_type_500_max)) {
			 PRINT_MSG ("ERROR: Incorrect HTTP code number %d\n", $1);
			 return 1;
	   }

	   snprintf (code, 4, "%d", $1);
	   dirs_table_set_handler_prop (current_config_entry, code, $2);
};

handler_option : T_SHOW id_list
{
	   linked_list_t *i;

	   i = $2;
	   while (i != NULL) {
			 if ((!strncasecmp (i->string, "date",  4)) ||
				(!strncasecmp (i->string, "size",  4)) ||
				(!strncasecmp (i->string, "group", 5)) ||
				(!strncasecmp (i->string, "owner", 5)))
			 {
				    char *lower;

				    lower = new_string_to_lowercase (i->string);
				    free (i->string);
				    i->string = lower;

				    cherokee_config_entry_set_handler_prop (current_config_entry, 
													   i->string, typed_int, INT_TO_POINTER(1), NULL);
			 } else {
				    PRINT_MSG ("ERROR: Unknown parameter '%s' for \"Show\"", i->string);
			 }
				
			 i = i->next;
	   }

	   free_linked_list ($2, free);
};


host_name : T_ID
          | T_IP 
          | T_DOMAIN_NAME
{
	   $$ = $1;
};

server : T_SERVER domain_list '{' 
{
	   linked_list_t *i = $2;
	   CHEROKEE_NEW(vsrv, virtual_server);

	   current_virtual_server = vsrv;
	   current_dirs_table     = &vsrv->dirs;

	   /* Add the virtual server to the list
	    */
	   list_add ((list_t *)vsrv, &SRV(server)->vservers);

	   /* Add default virtual server name
	    */
	   if (i->string != NULL) {
			 cherokee_buffer_t *name = current_virtual_server->name;

			 if (cherokee_buffer_is_empty (name)) {
				    cherokee_buffer_add_va (name, "%s", i->string);
			 }
	   }

	   /* Add all the alias to the table
	    */
	   while (i != NULL) {
			 cherokee_table_add (SRV(server)->vservers_ref, i->string, vsrv);
			 i = i->next;
	   }
	   free_linked_list ($2, NULL);

} server_lines '}' {

	   current_virtual_server = NULL;
	   current_dirs_table  = NULL;
};


extension : T_EXTENSION id_list '{'
{
	   /* Fill the tmp struct
	    */
	   extension_content_tmp.exts_list      = $2;
	   extension_content_tmp.vserver        = auto_virtual_server;
	   extension_content_tmp.entry          = config_entry_new (); /* new! */
	   extension_content_tmp.handler_name   = NULL;
	   extension_content_tmp.document_root  = NULL;

	   /* Extensions table is created under demand
	    */
	   if (extension_content_tmp.vserver->exts == NULL) {
			 ret_t ret;

			 ret = cherokee_exts_table_new (&extension_content_tmp.vserver->exts);
			 if (unlikely (ret != ret_ok)) {
				    PRINT_MSG_S ("ERROR: Couldn't instance a new exts table object\n");
				    return 1;
			 }
	   }
	   extension_content_tmp.exts = extension_content_tmp.vserver->exts;
} 
directory_options '}'
{
	   ret_t                   ret;
	   linked_list_t          *i;
	   cherokee_module_info_t *info;

	   /* Does this directory have a handler
	    */
	   if (extension_content_tmp.handler_name != NULL) {
			 int re;
			 re = load_module (&SRV(server)->loader, extension_content_tmp.handler_name, &info);
			 if (re != 0) return 1;
	   
			 cherokee_config_entry_set_handler (extension_content_tmp.entry, info);	   
	   }

	   /* Add "web_dir -> entry" in the dirs table
	    */
	   i = extension_content_tmp.exts_list;
	   while (i != NULL) {
			 ret = cherokee_exts_table_has (extension_content_tmp.exts, i->string);
			 if (ret != ret_not_found) {
				    PRINT_MSG ("ERROR: Extension '%s' was already set\n", i->string);
				    return 1;
			 }

			 ret = cherokee_exts_table_add (extension_content_tmp.exts,
									  i->string,
									  extension_content_tmp.entry);
			 if (ret != ret_ok) {
				    switch (ret) {
				    case ret_file_not_found:
						  PRINT_MSG ("ERROR: Can't load handler '%s': File not found\n",
								   extension_content_tmp.handler_name);
						  break;
				    default:
						  PRINT_MSG ("ERROR: Can't load handler '%s': Unknown error\n",
								   extension_content_tmp.handler_name);
				    }
			 }

			 i = i->next;
	   }

	   /* Clean
	    */
	   if (extension_content_tmp.document_root != NULL) {
			 free (extension_content_tmp.document_root);
			 extension_content_tmp.document_root = NULL;
	   }

	   extension_content_tmp.vserver       = NULL;
	   extension_content_tmp.exts          = NULL;
	   extension_content_tmp.entry         = NULL;
	   extension_content_tmp.handler_name  = NULL;

	   free_linked_list (extension_content_tmp.exts_list, free);
	   extension_content_tmp.exts_list = NULL;

	   current_config_entry = NULL;
};


directory : T_DIRECTORY fulldir '{' 
{
	   /* Fill the tmp struct
	    */
	   directory_content_tmp.directory_name = remove_last_slash($2);
	   directory_content_tmp.vserver        = auto_virtual_server;
	   directory_content_tmp.dirs           = auto_dirs_table;
	   directory_content_tmp.entry          = config_entry_new (); /* new! */
	   directory_content_tmp.handler_name   = NULL;
	   directory_content_tmp.document_root  = NULL;
} 
directory_options '}'
{
	   ret_t                   ret;
	   cherokee_module_info_t *info;

	   /* Set the document_root in the entry
	    */
	   if (directory_content_tmp.document_root != NULL) {
			 if (directory_content_tmp.entry->document_root == NULL)
				    cherokee_buffer_new (&directory_content_tmp.entry->document_root);
			 else 
				    cherokee_buffer_clean (directory_content_tmp.entry->document_root);

			 cherokee_buffer_add_va (directory_content_tmp.entry->document_root, "%s",
								directory_content_tmp.document_root);
	   }

	   /* Does this directory have a handler
	    */
	   if (directory_content_tmp.handler_name != NULL) {
			 int re;
			 re = load_module (&SRV(server)->loader, directory_content_tmp.handler_name, &info);
			 if (re != 0) return 1;
	   
			 cherokee_config_entry_set_handler (directory_content_tmp.entry, info);	   
	   }
	   
	   /* The root directory is treated as a special case, it is the
	    * default handler.  All the rest are going to be added into
	    * the directory table object.
	    */
	   if (strcmp (directory_content_tmp.directory_name, "/") == 0) {
			 directory_content_tmp.vserver->default_handler = 
				    directory_content_tmp.entry;

			 directory_content_tmp.vserver->default_handler->priority = 
				    CHEROKEE_CONFIG_PRIORITY_DEFAULT;
	   } else {
			 ret = cherokee_dirs_table_add (directory_content_tmp.dirs,
									  directory_content_tmp.directory_name,
									  directory_content_tmp.entry);

			 if (ret != ret_ok) {
				    switch (ret) {
				    case ret_file_not_found:
						  PRINT_MSG ("ERROR: Can't load handler '%s': File not found\n",
								   directory_content_tmp.handler_name);
						  break;
				    default:
						  PRINT_MSG ("ERROR: Can't load handler '%s': Unknown error\n",
								   directory_content_tmp.handler_name);
				    }
			 }

			 cherokee_dirs_table_relink (directory_content_tmp.dirs);
	   }

	   /* Clean
	    */
	   if (directory_content_tmp.document_root != NULL) {
			 free (directory_content_tmp.document_root);
			 directory_content_tmp.document_root = NULL;
	   }
	   directory_content_tmp.vserver       = NULL;
	   directory_content_tmp.dirs          = NULL;
	   directory_content_tmp.entry         = NULL;
	   directory_content_tmp.handler_name  = NULL;

	   current_config_entry = NULL;
};


request : T_REQUEST T_QSTRING '{'
{
	   /* Fill the tmp struct
	    */
	   request_content_tmp.vserver        = auto_virtual_server;
	   request_content_tmp.reqs           = auto_reqs_table;
	   request_content_tmp.entry          = reqs_list_entry_new (); /* new! */
	   request_content_tmp.handler_name   = NULL;
	   request_content_tmp.document_root  = NULL;
	   
	   cherokee_buffer_add (&(request_content_tmp.entry->request), $2, strlen($2));
} 
directory_options '}'
{
	   ret_t                   ret;
	   cherokee_module_info_t *info;

	   /* Does this directory have a handler
	    */
	   if (request_content_tmp.handler_name != NULL) {
			 int re;
			 re = load_module (&SRV(server)->loader, request_content_tmp.handler_name, &info);
			 if (re != 0) return 1;
	   
			 cherokee_config_entry_set_handler ((cherokee_config_entry_t *)request_content_tmp.entry, info);	   
	   }

	   /* Add the new entry
	    */
	   ret = cherokee_reqs_list_add (request_content_tmp.reqs, request_content_tmp.entry, SRV(server)->regexs);
	   if (ret != ret_ok) return 1;

	   /* Clean
	    */
	   if (request_content_tmp.document_root != NULL) {
			 free (request_content_tmp.document_root);
			 request_content_tmp.document_root = NULL;
	   }

	   request_content_tmp.vserver       = NULL;
	   request_content_tmp.reqs          = NULL;
	   request_content_tmp.entry         = NULL;
	   request_content_tmp.handler_name  = NULL;
}


directory_option : handler
{	   
	   directory_content_tmp.handler_name = $1.name;
	   extension_content_tmp.handler_name = $1.name;
	   request_content_tmp.handler_name   = $1.name;
};


directory_option : T_DOCUMENT_ROOT fulldir
{
	   if (directory_content_tmp.document_root != NULL) {
			 PRINT_MSG ("WARNING: Overwriting DocumentRoot '%s' by '%s'\n",
					  directory_content_tmp.document_root, $2);

			 free (directory_content_tmp.document_root);
	   }

	   directory_content_tmp.document_root = make_slash_end ($2);
};


directory_option : T_AUTH id_list '{'
{
	   linked_list_t               *i = $2;
	   cherokee_config_entry_t *entry = current_config_entry;

	   while (i != NULL) {
			 if (strncasecmp(i->string, "basic", 5) == 0) {
				    entry->authentication |= http_auth_basic;
			 } 
			 else if (strncasecmp(i->string, "digest", 6) == 0) {
				    entry->authentication |= http_auth_digest;			 
			 }
			 else {
				    PRINT_MSG ("ERROR: Unknown authentication type '%s'\n", i->string);
				    return 1;
			 }

			 i = i->next;
	   }

	   free_linked_list ($2, free);
} auth_options '}';


directory_option : T_ALLOW T_FROM ip_list 
{
	   ret_t ret;
	   linked_list_t *i, *prev;
	   CHEROKEE_NEW(n, access);

	   i = $3;
	   while (i != NULL) {
			 ret = cherokee_access_add (n, i->string);
			 if (ret != ret_ok) return 1;

			 free (i->string);
			 prev = i;
			 i = i->next;
			 free (prev);
	   }

	   current_config_entry->access = n;
};

directory_option : T_ONLY_SECURE
{
#ifndef HAVE_TLS
	   PRINT_MSG_S ("WARNING: Ignoring SSL configuration entry: \"OnlySecure\"\n");
#endif

	   current_config_entry->only_secure = true;
};

auth_option : T_NAME T_QSTRING 
{
	   cherokee_buffer_t *realm;

	   if (current_config_entry->auth_realm == NULL) 
			 cherokee_buffer_new (&current_config_entry->auth_realm);

	   realm = current_config_entry->auth_realm;


	   cherokee_buffer_add (realm, $2, strlen($2));
	   free ($2);
};

auth_option : T_USER id_list
{
	   linked_list_t *i;

	   if (current_config_entry->users == NULL) {
			 cherokee_table_new (&current_config_entry->users);
	   }

	   i = $2;
	   while (i!=NULL) {
			 linked_list_t *prev;

			 cherokee_table_add (current_config_entry->users, i->string, NULL);

			 free(i->string);
			 prev = i;
			 i = i->next;
			 free(prev);
	   }	   
};

auth_option : T_METHOD T_ID maybe_auth_option_params
{
	   ret_t ret;
	   cherokee_module_info_t           *info;
	   cherokee_module_info_validator_t *vinfo;
	   cherokee_config_entry_t          *entry = current_config_entry;

	   /* Load the module
	    */
	   ret = cherokee_module_loader_load (&SRV(server)->loader, $2);
	   if (ret != ret_ok) {
			 PRINT_MSG ("ERROR: Can't load validator module '%s'\n", $2);
			 return 1;
	   }

	   cherokee_module_loader_get_info  (&SRV(server)->loader, $2, &info);

	   if (info->type != cherokee_validator) {
			 PRINT_MSG ("ERROR: %s is not a validator module!!\n", $2);
	   }

	   vinfo = (cherokee_module_info_validator_t *)info;
	   
	   /* Check that the method is supported
	    */
	   if ((entry->authentication & vinfo->valid_methods) != entry->authentication) {
			 PRINT_MSG ("ERROR: The module %s doesn't support all the authentication methods that you configured\n", $2);
			 return 1;
	   }

	   entry->validator_new_func = info->new_func;
};

maybe_auth_option_params : '{' auth_option_params '}'
                         |
                         ;

auth_option_params : 
                   | auth_option_params auth_option_param
                   ;

auth_option_param : T_PASSWDFILE fulldir 
{ dirs_table_set_validator_prop (current_config_entry, "file", $2); };


userdir : T_USERDIR T_ID 
{
	   int   len;
	   char *tmp;
	   cherokee_virtual_server_t *vsrv = auto_virtual_server;

	   /* Set the users public directory
	    */
	   if (!cherokee_buffer_is_empty (vsrv->userdir)) {
			 PRINT_MSG ("WARNING: Overwriting userdir '%s'\n", vsrv->userdir->buf);
			 cherokee_buffer_clean (vsrv->userdir);
	   }

	   len = strlen($2);
	   tmp = make_finish_with_slash ($2, &len);
	   cherokee_buffer_add (vsrv->userdir, tmp, len);

	   /* Set the plugin table reference
	    */
	   current_dirs_table = vsrv->userdir_dirs;

}  '{' directories '}' {

	   /* Remove the references
	    */
	   current_dirs_table = NULL;
};


directoryindex : T_DIRECTORYINDEX id_path_list
{
	   linked_list_t             *i    = $2;
	   cherokee_virtual_server_t *vsrv = auto_virtual_server;

	   while (i != NULL) {
			 cherokee_list_add_tail (&vsrv->index_list, i->string);
			 i = i->next;
	   }

	   free_linked_list ($2, NULL);
};


maybe_handlererror_options : 
                           | '{' handler_options '}';

errorhandler : T_ERROR_HANDLER T_ID
{
	   ret_t                      ret;
	   cherokee_module_info_t    *info;
	   cherokee_virtual_server_t *vsrv = auto_virtual_server;

	   /* Load the module
	    */
	   ret = cherokee_module_loader_load (&SRV(server)->loader, $2);
	   if (ret < ret_ok) {
			 PRINT_MSG ("ERROR: Loading module '%s'\n", $2);
			 return 1;
	   }

	   ret = cherokee_module_loader_get_info (&SRV(server)->loader, $2, &info);
	   if (ret < ret_ok) {
			 PRINT_MSG ("ERROR: Loading module '%s'\n", $2);
			 return 1;
	   }
	   
	   /* Remove the old (by default) error handler and cretate a new one
	    */
	   vsrv->error_handler = config_entry_new();

	   /* Setup the loaded module
	    */
	   cherokee_config_entry_set_handler (vsrv->error_handler, info);
} 
maybe_handlererror_options;

%%

