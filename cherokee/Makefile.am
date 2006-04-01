## Cherokee: cherokee/Makefile.am  -*- makefile -*-

INCLUDES = \
$(TLS_CFLAGS) \
$(PTHREAD_CFLAGS) \
-I$(top_srcdir) \
-DCHEROKEE_COMPILATION \
-DCHEROKEE_PLUGINDIR="\"$(plugindir)\"" \
-DCHEROKEE_DEPSDIR=\""$(cherokeedepsdir)"\" \
-DCHEROKEE_DATADIR=\""$(cherokeedatadir)"\" \
-DCHEROKEE_CONFDIR=\""$(sysconfdir)/cherokee"\" \
-DCHEROKEE_ICONSDIR=\""$(prefix)/share/cherokee/icons"\"

#
# Scripts
#
bin_SCRIPTS = cherokee-panic

#
# Data
#
cherokeedepsdir = "$(datadir)/cherokee/deps"
cherokeedeps_DATA = \
nn.deps \
common.deps \
combined.deps \
error_redir.deps \
phpcgi.deps

cherokeedatadir = "$(datadir)/cherokee"
cherokeedata_DATA = mime_types.txt

#
# External sources
#
zlib_src =     \
zlib/deflate.c \
zlib/deftree.c \
zlib/defutil.h \
zlib/zconf.h   \
zlib/zlib.h    \
zlib/zutil.h

dict_src =          \
dict/dict_generic.h \
dict/dict_generic.c \
dict/sp_tree.h      \
dict/sp_tree.c      \
dict/dict_private.h

avl_src = \
avl/avl.h \
avl/avl.c

pcre_src = \
pcre/get.c \
pcre/pcre.h \
pcre/pcre.c \
pcre/internal.h \
pcre/chartables.c \
pcre/configuration.h

getopt_src =  \
getopt/getopt.h  \
getopt/getopt.c  \
getopt/getopt1.c \
getopt/gettext.h


if !HAVE_GETOPT_LONG
internal_getopt_src=$(getopt_src)
endif

#
# LDFLAGS for libraries
#
if PLATFORM_WIN32
no_undefined = -no-undefined -Wl,--export-all-symbols
module_ldflags_w32 = -lcherokee-base -lcherokee-config -lcherokee-server
endif

common_ldflags = \
$(no_undefined) \
$(LIBTOOL_EXPORT_OPTIONS)

#
# LDFLAGS for modules = flags for libraries + more
#
module_ldflags = \
$(common_ldflags) \
-module \
-avoid-version \
-export-dynamic \
$(module_ldflags_w32) 


#
# Config stuff
#
module_read_config = \
read_config_grammar.y \
read_config_scanner.l \
module_read_config.h \
module_read_config.c

libplugin_read_config_la_LDFLAGS = $(module_ldflags)
libplugin_read_config_la_SOURCES = $(module_read_config)

if STATIC_MODULE_READ_CONFIG
static_module_read_config_src = $(module_read_config)
else
dynamic_module_read_config_lib = libplugin_read_config.la
endif


#
# Handler server_info
#
handler_server_info = \
handler_server_info.c \
handler_server_info.h \
logo.inc 

libplugin_server_info_la_LDFLAGS = $(module_ldflags) 
libplugin_server_info_la_SOURCES = $(handler_server_info)

if STATIC_HANDLER_SERVER_INFO
static_handler_server_info_src = $(handler_server_info)
else
dynamic_handler_server_info_lib = libplugin_server_info.la
endif


#
# Handler file
#
handler_file = \
handler_file.c \
handler_file.h

libplugin_file_la_LDFLAGS = $(module_ldflags)  
libplugin_file_la_SOURCES = $(handler_file)

if STATIC_HANDLER_FILE
static_handler_file_src = $(handler_file)
else
dynamic_handler_file_lib = libplugin_file.la
endif


#
# Handler dirlist
#
handler_dirlist = \
handler_dirlist.c \
handler_dirlist.h

libplugin_dirlist_la_LDFLAGS = $(module_ldflags)  
libplugin_dirlist_la_SOURCES = $(handler_dirlist)

if STATIC_HANDLER_DIRLIST
static_handler_dirlist_src = $(handler_dirlist)
else
dynamic_handler_dirlist_lib = libplugin_dirlist.la
endif


#
# Handler cgi
#
if !STATIC_HANDLER_CGI
handlercgi_cgi = handler_cgi_base.h handler_cgi_base.c
endif

handler_cgi = \
$(handlercgi_cgi) \
handler_cgi.c \
handler_cgi.h

libplugin_cgi_la_LDFLAGS = $(module_ldflags)  
libplugin_cgi_la_SOURCES = $(handler_cgi)

if STATIC_HANDLER_CGI
static_handler_cgi_src = $(handler_cgi)
else
dynamic_handler_cgi_lib = libplugin_cgi.la
endif


#
# Handler scgi
#
if !STATIC_HANDLER_SCGI
handlerscgi_cgi = handler_cgi_base.h handler_cgi_base.c
endif

handler_scgi = \
$(handlerscgi_cgi) \
handler_scgi.h \
handler_scgi.c

libplugin_scgi_la_LDFLAGS = $(module_ldflags)  
libplugin_scgi_la_SOURCES = $(handler_scgi)

if STATIC_HANDLER_SCGI
static_handler_scgi_src = $(handler_scgi)
else
dynamic_handler_scgi_lib = libplugin_scgi.la
endif


#
# Handler fcgi
#
if !STATIC_HANDLER_FCGI
handlerfcgi_cgi = handler_cgi_base.h handler_cgi_base.c
endif

handler_fcgi = \
$(handlerfcgi_cgi) \
fastcgi.h \
handler_fcgi.h \
handler_fcgi.c

libplugin_fcgi_la_LDFLAGS = $(module_ldflags)  
libplugin_fcgi_la_SOURCES = $(handler_fcgi)

if STATIC_HANDLER_FCGI
static_handler_fcgi_src = $(handler_fcgi)
else
dynamic_handler_fcgi_lib = libplugin_fcgi.la
endif


#
# Handler fastcgi
#
if !STATIC_HANDLER_FASTCGI
handlerfastcgi_cgi = handler_cgi_base.h handler_cgi_base.c
endif

handler_fastcgi = \
$(handlerfastcgi_cgi) \
fastcgi.h \
fcgi_dispatcher.h \
fcgi_dispatcher.c \
fcgi_manager.h \
fcgi_manager.c \
handler_fastcgi.c \
handler_fastcgi.h 

libplugin_fastcgi_la_LDFLAGS = $(module_ldflags)  
libplugin_fastcgi_la_SOURCES = $(handler_fastcgi)

if STATIC_HANDLER_FASTCGI
static_handler_fastcgi_src = $(handler_fastcgi)
else
dynamic_handler_fastcgi_lib = libplugin_fastcgi.la
endif


#
# Handler phpcgi
#
handler_phpcgi = \
handler_phpcgi.c \
handler_phpcgi.h

if PLATFORM_WIN32
phpcgi_ldflags_win32 = -lplugin_cgi
endif

libplugin_phpcgi_la_LDFLAGS = $(module_ldflags) $(phpcgi_ldflags_win32)
libplugin_phpcgi_la_SOURCES = $(handler_phpcgi)

if STATIC_HANDLER_PHPCGI
static_handler_phpcgi_src = $(handler_phpcgi)
else
dynamic_handler_phpcgi_lib = libplugin_phpcgi.la
endif


#
# Handler redir
#
handler_redir = \
handler_redir.c \
handler_redir.h

libplugin_redir_la_LDFLAGS = $(module_ldflags)  
libplugin_redir_la_SOURCES = $(handler_redir)

if STATIC_HANDLER_REDIR
static_handler_redir_src = $(handler_redir)
else
dynamic_handler_redir_lib = libplugin_redir.la
endif


#
# Handler error_redir
#
handler_error_redir = \
handler_error_redir.c \
handler_error_redir.h

if PLATFORM_WIN32
error_redir_ldflags_win32 = -lplugin_redir
endif

libplugin_error_redir_la_LDFLAGS = $(module_ldflags) $(error_redir_ldflags_win32)
libplugin_error_redir_la_SOURCES = $(handler_error_redir)

if STATIC_HANDLER_ERROR_REDIR
static_handler_error_redir_src = $(handler_error_redir)
else
dynamic_handler_error_redir_lib = libplugin_error_redir.la
endif


#
# Handler common
#
handler_common = \
handler_common.c \
handler_common.h

if PLATFORM_WIN32
common_ldflags_win32 = -lplugin_dirlist -lplugin_file
endif

libplugin_common_la_LDFLAGS = $(module_ldflags) $(common_ldflags_win32)
libplugin_common_la_SOURCES = $(handler_common)

if STATIC_HANDLER_COMMON
static_handler_common_src = $(handler_common)
else
dynamic_handler_common_lib = libplugin_common.la
endif


#
# Handler nn
#
handler_nn = \
levenshtein_distance.c \
levenshtein_distance.h \
handler_nn.c \
handler_nn.h

if PLATFORM_WIN32
nn_ldflags_win32 = -lplugin_common -lplugin_redir
endif

libplugin_nn_la_LDFLAGS = $(module_ldflags) $(nn_ldflags_win32)
libplugin_nn_la_SOURCES = $(handler_nn)

if STATIC_HANDLER_NN
static_handler_nn_src = $(handler_nn)
else
dynamic_handler_nn_lib = libplugin_nn.la
endif


#
# Handler admin
#
config_common = \
connection_info.h \
connection_info.c

handler_admin = \
handler_admin.c \
handler_admin.h \
admin_server.h \
admin_server.c 

libplugin_admin_la_LDFLAGS = $(module_ldflags)  
libplugin_admin_la_SOURCES = $(handler_admin)
libplugin_admin_la_LIBADD  = libcherokee-config.la libcherokee-client.la  

if STATIC_HANDLER_ADMIN
static_handler_admin_src = $(handler_admin) $(config_common)
else
dynamic_handler_admin_lib = libplugin_admin.la
endif


#
# Encoder GZip
#
encoder_gzip = \
encoder_gzip.h \
encoder_gzip.c

libplugin_gzip_la_LDFLAGS = $(module_ldflags)  
libplugin_gzip_la_SOURCES = $(encoder_gzip)

if STATIC_ENCODER_GZIP
static_encoder_gzip_src = $(encoder_gzip)
else
dynamic_encoder_gzip_lib = libplugin_gzip.la
endif


#
# Logger NCSA
#
logger_ncsa = \
logger_ncsa.h \
logger_ncsa.c

libplugin_ncsa_la_LDFLAGS = $(module_ldflags)  
libplugin_ncsa_la_SOURCES = $(logger_ncsa)

if STATIC_LOGGER_NCSA
static_logger_ncsa_src = $(logger_ncsa)
else
dynamic_logger_ncsa_lib = libplugin_ncsa.la
endif


#
# Logger Combined
#
logger_combined = \
logger_combined.h \
logger_combined.c

if PLATFORM_WIN32
combined_ldflags_win32 = -lplugin_ncsa
endif

libplugin_combined_la_LDFLAGS = $(module_ldflags) $(combined_ldflags_win32)
libplugin_combined_la_SOURCES = $(logger_combined)

if STATIC_LOGGER_COMBINED
static_logger_combined_src = $(logger_combined)
else
dynamic_logger_combined_lib = libplugin_combined.la
endif


#
# Logger W3C
#
logger_w3c = \
logger_w3c.h \
logger_w3c.c

libplugin_w3c_la_LDFLAGS = $(module_ldflags)  
libplugin_w3c_la_SOURCES = $(logger_w3c)

if STATIC_LOGGER_W3C
static_logger_w3c_src = $(logger_w3c)
else
dynamic_logger_w3c_lib = libplugin_w3c.la
endif


#
# Validator PAM
#
if HAVE_PAM
validator_pam = \
validator_pam.c \
validator_pam.h

libplugin_pam_la_LDFLAGS = $(module_ldflags) 
libplugin_pam_la_SOURCES = $(validator_pam)
libplugin_pam_la_LIBADD  = -lpam

if STATIC_VALIDATOR_PAM
static_validator_pam_src = $(validator_pam)
static_validator_pam_lib = -lpam
else
dynamic_validator_pam_lib = libplugin_pam.la
endif
endif

#
# Validator htpasswd
#
if HAVE_CRYPT
validator_htpasswd = \
validator_htpasswd.c \
validator_htpasswd.h

libplugin_htpasswd_la_LDFLAGS = $(module_ldflags) 
libplugin_htpasswd_la_SOURCES = $(validator_htpasswd)
libplugin_htpasswd_la_LIBADD  = $(CRYPT_LIBS)
libplugin_htpasswd_la_CFLAGS  = $(CRYPT_CFLAGS)

if STATIC_VALIDATOR_HTPASSWD
static_validator_htpasswd_src   = $(validator_htpasswd)
static_validator_htpasswd_lib   = $(CRYPT_LIBS)
static_validator_htpasswd_flags = $(CRYPT_CFLAGS)
else
dynamic_validator_htpasswd_lib = libplugin_htpasswd.la
endif
endif

#
# Validator plain text
#
validator_plain = \
validator_plain.c \
validator_plain.h

libplugin_plain_la_LDFLAGS = $(module_ldflags) 
libplugin_plain_la_SOURCES = $(validator_plain)

if STATIC_VALIDATOR_PLAIN
static_validator_plain_src = $(validator_plain)
else
dynamic_validator_plain_lib = libplugin_plain.la
endif


#
# Validator htdigest
#
validator_htdigest = \
validator_htdigest.c \
validator_htdigest.h

libplugin_htdigest_la_LDFLAGS = $(module_ldflags) 
libplugin_htdigest_la_SOURCES = $(validator_htdigest)

if STATIC_VALIDATOR_HTDIGEST
static_validator_htdigest_src = $(validator_htdigest)
else
dynamic_validator_htdigest_lib = libplugin_htdigest.la
endif



#
# Yacc and Lex paramters
# -d Generate .h file
# -l Don't use #line
# -t Debug
AM_YFLAGS=-d -l -t
AM_LFLAGS=-i


if USE_INTERNAL_PCRE
internal_pcre_src=$(pcre_src)
else
external_pcre_lib=-lpcre
endif


lib_LTLIBRARIES = \
libcherokee-base.la \
libcherokee-client.la \
libcherokee-server.la \
libcherokee-config.la


# the macro puts -pthread into cflags, but we want to link
# with this flag too, so stick it into the ldflags if it's there...
# extra cflags at link time can't hurt
libcherokee_base_la_LDFLAGS = $(common_ldflags)
libcherokee_base_la_LIBADD = \
$(TLS_LIBS) \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS) \
$(external_pcre_lib)

libcherokee_client_la_LDFLAGS = $(common_ldflags)
libcherokee_client_la_LIBADD = \
$(TLS_LIBS) \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS) \
libcherokee-base.la

libcherokee_server_la_LDFLAGS = $(common_ldflags)
libcherokee_server_la_CFLAGS = \
$(static_validator_htpasswd_flags)
libcherokee_server_la_LIBADD = \
$(TLS_LIBS) \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS) \
$(static_validator_pam_lib) \
$(static_validator_htpasswd_lib) \
libcherokee-base.la

libcherokee_config_la_LDFLAGS = $(common_ldflags)
libcherokee_config_la_LIBADD = \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS) \
libcherokee-base.la \
libcherokee-client.la \
libcherokee-server.la


if COMPILE_EPOLL
poll_epoll_src = fdpoll-epoll.c
endif

if COMPILE_KQUEUE
poll_kqueue_src = fdpoll-kqueue.c
endif

if COMPILE_PORT
poll_port_src = fdpoll-port.c
endif

if COMPILE_POLL
poll_poll_src = fdpoll-poll.c
endif

if COMPILE_SELECT
poll_select_src = fdpoll-select.c
endif

if COMPILE_WIN32_SELECT
poll_win32_src = fdpoll-win32.c
endif

if PLATFORM_WIN32
win32_src = \
unix4win32.h \
unix4win32.c \
win32_misc.h \
win32_misc.c
endif

libcherokee_base_la_SOURCES = \
$(avl_src) \
$(zlib_src) \
$(dict_src) \
$(internal_pcre_src) \
$(internal_getopt_src) \
$(poll_poll_src) \
$(poll_epoll_src) \
$(poll_kqueue_src) \
$(poll_port_src) \
$(poll_select_src) \
$(poll_win32_src) \
$(win32_src) \
cherokee.h \
http.h \
http.c \
list.h \
list_merge_sort.h \
list_merge_sort.c \
list_ext.h \
list_ext.c \
ncpus.h \
ncpus.c \
crc32.h \
crc32.c \
macros.h \
common.h \
common-internal.h \
util.h \
util.c \
buffer.h \
buffer.c \
table.h \
table-protected.h \
table.c \
dict.h \
dict.c \
buffer_escape.h \
buffer_escape.c \
socket.h \
socket.c \
fdpoll.h \
fdpoll-protected.h \
fdpoll.c \
mime.h \
mime-protected.h \
mime.c \
mime_entry.c \
matching_list.h \
matching_list.c \
iocache.h \
iocache.c \
md5.h \
md5.c \
md5crypt.h \
md5crypt.c \
sha1.h \
sha1.c \
match.h \
match.c \
tdate_parse.h \
tdate_parse.c \
header.h \
header-protected.h \
header.c \
session_cache.h \
session_cache.c \
access.h \
access.c \
regex.h \
regex.c \
resolv_cache.h \
resolv_cache.c \
typed_table.h \
typed_table.c 

libcherokee_client_la_SOURCES = \
url.h \
url.c \
request.h \
request.c \
downloader.h \
downloader-protected.h \
downloader.c 


if STATIC_HANDLER_FCGI
   common_cgi = handler_cgi_base.h \
	     	 handler_cgi_base.c
endif

if STATIC_HANDLER_FASTCGI
   common_cgi = handler_cgi_base.h \
	     	 handler_cgi_base.c
endif

if STATIC_HANDLER_CGI
   common_cgi = handler_cgi_base.h \
	     	 handler_cgi_base.c
endif

if STATIC_HANDLER_SCGI
   common_cgi = handler_cgi_base.h \
	     	 handler_cgi_base.c
endif


libcherokee_server_la_SOURCES = \
icons_grammar.y \
icons_scanner.l \
\
$(static_module_read_config_src) \
$(static_handler_file_src) \
$(static_handler_admin_src) \
$(static_handler_dirlist_src) \
$(static_handler_scgi_src) \
$(static_handler_fcgi_src) \
$(static_handler_fastcgi_src) \
$(static_handler_redir_src) \
$(static_handler_error_redir_src) \
$(static_handler_cgi_src) \
$(static_handler_phpcgi_src) \
$(static_handler_common_src) \
$(static_handler_nn_src) \
$(static_handler_server_info_src) \
\
$(static_logger_w3c_src) \
$(static_logger_ncsa_src) \
$(static_logger_combined_src) \
\
$(static_encoder_gzip_src) \
\
$(static_validator_pam_src) \
$(static_validator_plain_src) \
$(static_validator_htdigest_src) \
$(static_validator_htpasswd_src) \
\
$(common_cgi) \
\
connection.h \
connection-protected.h \
connection.c \
handler.h \
handler.c \
config_entry.h \
config_entry.c \
dirs_table.h \
dirs_table.c \
exts_table.h \
exts_table.c \
reqs_list.h \
reqs_list.c \
reqs_list_entry.h \
reqs_list_entry.c \
server-protected.h \
server.h \
server.c \
virtual_server.h \
virtual_server.c \
encoder.h \
encoder.c \
encoder_table.h \
encoder_table.c \
logger.h \
logger.c \
logger_table.h \
logger_table.c \
thread.h \
thread.c \
module.h \
module.c \
validator.h \
validator.c \
module_loader.h \
module_loader.c \
icons.h \
icons.c \
handler_error.c \
handler_error.h \
nonce.h \
nonce.c \
post.h \
post.c \
ext_source.h \
ext_source.c

libcherokee_config_la_SOURCES = \
$(config_common) \
admin_client.h \
admin_client.c 


#
# Header files
#
libcherokeeincludedir = $(includedir)/cherokee
libcherokeeinclude_HEADERS = \
cherokee.h \
macros.h \
common.h \
util.h \
buffer.h\
fdpoll.h \
table.h \
http.h \
mime.h \
mime_entry.h \
list.h \
list_merge_sort.h \
url.h \
header.h \
resolv_cache.h \
typed_table.h \
\
server.h \
module.h \
logger.h \
handler.h \
encoder.h \
connection.h \
module_loader.h \
nonce.h \
\
downloader.h \
\
admin_client.h \
connection_info.h


# automake isn't too clever about "non-standard" use of lex and yacc
# Icons:
icons_scanner.c : icons_scanner.l
	$(LEX) -Pyy_icons_ -i -o$@ $<

icons_grammar.h : icons_grammar.c
icons_grammar.c : icons_grammar.y
	$(YACC) -dv -p yy_icons_ -o $@ $<


#
# Cherokee plugins
#
plugindir = "$(libdir)/cherokee"
plugin_LTLIBRARIES = \
$(dynamic_module_read_config_lib) \
$(dynamic_handler_cgi_lib) \
$(dynamic_handler_phpcgi_lib) \
$(dynamic_handler_server_info_lib) \
$(dynamic_handler_file_lib) \
$(dynamic_handler_admin_lib) \
$(dynamic_handler_dirlist_lib) \
$(dynamic_handler_scgi_lib) \
$(dynamic_handler_fcgi_lib) \
$(dynamic_handler_fastcgi_lib) \
$(dynamic_handler_redir_lib) \
$(dynamic_handler_error_redir_lib) \
$(dynamic_handler_common_lib) \
$(dynamic_handler_nn_lib) \
$(dynamic_encoder_gzip_lib) \
$(dynamic_logger_ncsa_lib) \
$(dynamic_logger_combined_lib) \
$(dynamic_logger_w3c_lib) \
$(dynamic_validator_pam_lib) \
$(dynamic_validator_plain_lib) \
$(dynamic_validator_htdigest_lib) \
$(dynamic_validator_htpasswd_lib)


#
# Main binary: Cherokee server
#
sbin_PROGRAMS = cherokee

cherokee_SOURCES = main.c
cherokee_LDADD   = \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS) \
libcherokee-base.la \
libcherokee-server.la
cherokee_LDFLAGS = -export-dynamic

#
# Log rotate utility
#
bin_PROGRAMS = cherokee_logrotate

cherokee_logrotate_SOURCES = cherokee_logrotate.c
cherokee_logrotate_LDADD = libcherokee-base.la libcherokee-client.la libcherokee-server.la libcherokee-config.la


#
# Test programs
#
noinst_PROGRAMS = cherokee_admin

cherokee_admin_SOURCES = cherokee_admin.c 
cherokee_admin_LDADD = libcherokee-config.la libcherokee-base.la libcherokee-client.la 

##cherokee_table_test_SOURCES = cherokee_table_test.c
##cherokee_table_test_LDADD = libcherokee.la

#noinst_PROGRAMS = cherokee_modules cherokee_base64 cherokee_headers cherokee_fastcgi

#cherokee_modules_SOURCES = cherokee_modules.c
#cherokee_modules_LDADD = libcherokee.la

#cherokee_base64_SOURCES = cherokee_base64.c
#cherokee_base64_LDADD = libcherokee.la

#cherokee_headers_SOURCES = cherokee_headers.c
#cherokee_headers_LDADD = libcherokee.la

#cherokee_fastcgi_SOURCES = cherokee_fastcgi.c
#cherokee_fastcgi_LDADD = libcherokee.la 
#cherokee_fastcgi_LDFLAGS = -lfcgi

#cherokee_ip_SOURCES = cherokee_ip.c
#cherokee_ip_LDADD = libcherokee.la

#cherokee_hex_SOURCES = cherokee_hex.c
#cherokee_hex_LDADD = libcherokee-base.la

#cherokee_short_path = cherokee_short_path.c
#cherokee_short_path_LDADD = libcherokee-base.la


CLEANFILES = \
read_config_grammar.h \
read_config_grammar.c \
read_config_scanner.c \
icons_grammar.h \
icons_grammar.c \
icons_scanner.c \
config.h

WINDOWS_PORT_FILES = \
Makefile.MingW \
Makefile.MingW.in \
unix4win32.h \
unix4win32.c \
win32_misc.h \
win32_misc.c

EMBEDDED_VERSION_FILES = \
Makefile.embedded \
read_config_embedded.h \
read_config_embedded.c

EXTRA_DIST = \
$(pcre_src) \
$(win32_src) \
$(getopt_src) \
$(bin_SCRIPTS) \
$(cherokeedeps_DATA) \
$(cherokeedata_DATA) \
$(WINDOWS_PORT_FILES) \
$(EMBEDDED_VERSION_FILES) \
read_config_grammar.h \
loader.autoconf.h \
loader.autoconf.inc
