## Cherokee: cherokee/Makefile.am  -*- makefile -*-

INCLUDES = \
-I${top_srcdir} \
$(TLS_CFLAGS) \
$(PTHREAD_CFLAGS)

#
# Scripts
#
bin_SCRIPTS = cherokee-panic

#
# Data
#
cherokeedepsdir = "$(datadir)/cherokee/deps"
cherokeedeps_DATA = \
common.deps \
combined.deps \
error_redir.deps \
phpcgi.deps \
scgi.deps \
fcgi.deps \
fastcgi.deps \
cgi.deps

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

pcre_src = \
pcre/ucp.h \
pcre/pcre.h \
pcre/local_config.h \
pcre/pcre_internal.h \
pcre/pcre_try_flipped.c \
pcre/pcre_chartables.c \
pcre/pcre_fullinfo.c \
pcre/pcre_newline.c \
pcre/pcre_globals.c \
pcre/pcre_compile.c \
pcre/pcre_tables.c \
pcre/pcre_exec.c \
pcre/pcre_get.c 

getopt_src =  \
getopt/getopt.h  \
getopt/getopt.c  \
getopt/getopt1.c \
getopt/gettext.h

#
# Check whether external libraries can be used
#
if !HAVE_GETOPT_LONG
internal_getopt_src=$(getopt_src)
endif

if USE_INTERNAL_PCRE
internal_pcre_src=$(pcre_src)
else
external_pcre_lib=-lpcre
endif

#
# LDFLAGS for libraries
#
library_soname = \
-version-info $(CHEROKEE_CURRENT):$(CHEROKEE_REVISION):$(CHEROKEE_AGE)

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
# Rule directory
#
rule_directory = \
rule_directory.c \
rule_directory.h

libplugin_directory_la_LDFLAGS = $(module_ldflags)  
libplugin_directory_la_SOURCES = $(rule_directory)

if STATIC_RULE_DIRECTORY
static_rule_directory_src = $(rule_directory)
else
dynamic_rule_directory_lib = libplugin_directory.la
endif


#
# Rule extensions
#
rule_extensions = \
rule_extensions.c \
rule_extensions.h

libplugin_extensions_la_LDFLAGS = $(module_ldflags)  
libplugin_extensions_la_SOURCES = $(rule_extensions)

if STATIC_RULE_EXTENSIONS
static_rule_extensions_src = $(rule_extensions)
else
dynamic_rule_extensions_lib = libplugin_extensions.la
endif


#
# Rule request
#
rule_request = \
rule_request.c \
rule_request.h

libplugin_request_la_LDFLAGS = $(module_ldflags)  
libplugin_request_la_SOURCES = $(rule_request)

if STATIC_RULE_REQUEST
static_rule_request_src = $(rule_request)
else
dynamic_rule_request_lib = libplugin_request.la
endif


#
# Rule header
#
rule_header = \
rule_header.c \
rule_header.h

libplugin_header_la_LDFLAGS = $(module_ldflags)  
libplugin_header_la_SOURCES = $(rule_header)

if STATIC_RULE_HEADER
static_rule_header_src = $(rule_header)
else
dynamic_rule_header_lib = libplugin_header.la
endif


#
# Rule geoip
#
if HAVE_GEOIP
rule_geoip = \
rule_geoip.c \
rule_geoip.h

libplugin_geoip_la_LDFLAGS = $(module_ldflags)  
libplugin_geoip_la_SOURCES = $(rule_geoip)
libplugin_geoip_la_LIBADD  = -lGeoIP

if STATIC_RULE_HEADER
static_rule_geoip_src = $(rule_geoip)
static_rule_geoip_lib = -lGeoIP
else
dynamic_rule_geoip_lib = libplugin_geoip.la
endif
endif


#
# Rule not
#
rule_not = \
rule_not.c \
rule_not.h

libplugin_not_la_LDFLAGS = $(module_ldflags)  
libplugin_not_la_SOURCES = $(rule_not)

if STATIC_RULE_NOT
static_rule_not_src = $(rule_not)
else
dynamic_rule_not_lib = libplugin_not.la
endif


#
# Rule and
#
rule_and = \
rule_and.c \
rule_and.h

libplugin_and_la_LDFLAGS = $(module_ldflags)  
libplugin_and_la_SOURCES = $(rule_and)

if STATIC_RULE_AND
static_rule_and_src = $(rule_and)
else
dynamic_rule_and_lib = libplugin_and.la
endif


#
# Rule or
#
rule_or = \
rule_or.c \
rule_or.h

libplugin_or_la_LDFLAGS = $(module_ldflags)  
libplugin_or_la_SOURCES = $(rule_or)

if STATIC_RULE_OR
static_rule_or_src = $(rule_or)
else
dynamic_rule_or_lib = libplugin_or.la
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
libplugin_server_info_la_LIBADD  = libcherokee-config.la

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
libplugin_cgi_la_LIBADD  = $(dynamic_handler_file_lib)

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
libplugin_scgi_la_LIBADD  = $(dynamic_handler_file_lib)

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
libplugin_fcgi_la_LIBADD  = $(dynamic_handler_file_lib)

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
libplugin_fastcgi_la_LIBADD  = $(dynamic_handler_file_lib)

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
libplugin_phpcgi_la_LIBADD  = $(dynamic_handler_file_lib)

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
# Handler error_nn
#
handler_error_nn = \
levenshtein_distance.c \
levenshtein_distance.h \
handler_error_nn.c \
handler_error_nn.h

libplugin_error_nn_la_LDFLAGS = $(module_ldflags)
libplugin_error_nn_la_SOURCES = $(handler_error_nn)

if STATIC_HANDLER_ERROR_NN
static_handler_error_nn_src = $(handler_error_nn)
else
dynamic_handler_error_nn_lib = libplugin_error_nn.la
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
# Handler proxy
#
handler_proxy = \
handler_proxy.c \
handler_proxy.h

libplugin_proxy_la_LDFLAGS = $(module_ldflags)  
libplugin_proxy_la_SOURCES = $(handler_proxy)
libplugin_proxy_la_LIBADD  = libcherokee-client.la

if STATIC_HANDLER_PROXY
static_handler_proxy_src = $(handler_proxy)
static_handler_proxy_lib = libcherokee-client.la
else
dynamic_handler_proxy_lib = libplugin_proxy.la
endif


#
# Handler mirror
#
handler_mirror = \
handler_mirror.c \
handler_mirror.h

libplugin_mirror_la_LDFLAGS = $(module_ldflags)  
libplugin_mirror_la_SOURCES = $(handler_mirror)
libplugin_mirror_la_LIBADD  = libcherokee-client.la  

if STATIC_HANDLER_MIRROR
static_handler_mirror_src = $(handler_mirror)
else
dynamic_handler_mirror_lib = libplugin_mirror.la
endif


#
# Handler DB Slayer
#
handler_dbslayer = \
dwriter.c \
dwriter.h \
handler_dbslayer.h \
handler_dbslayer.c

libplugin_dbslayer_la_LDFLAGS = $(module_ldflags) $(MYSQL_LDFLAGS)
libplugin_dbslayer_la_SOURCES = $(handler_dbslayer)
libplugin_dbslayer_la_CFLAGS  = $(MYSQL_CFLAGS)

if HAVE_MYSQL
if STATIC_HANDLER_DBSLAYER
static_handler_dbslayer_src   = $(handler_dbslayer)
static_handler_dbslayer_lib   = $(MYSQL_LDFLAGS)
static_handler_dbslayer_flags = $(MYSQL_CFLAGS)
else
dynamic_handler_dbslayer_lib = libplugin_dbslayer.la
endif
endif


#
# Handler custom_error
#
handler_custom_error = \
handler_custom_error.c \
handler_custom_error.h

libplugin_custom_error_la_LDFLAGS = $(module_ldflags)  
libplugin_custom_error_la_SOURCES = $(handler_custom_error)
libplugin_custom_error_la_LIBADD  = libcherokee-client.la  

if STATIC_HANDLER_CUSTOM_ERROR
static_handler_custom_error_src = $(handler_custom_error)
else
dynamic_handler_custom_error_lib = libplugin_custom_error.la
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
# Encoder Deflate
#
encoder_deflate = \
encoder_deflate.h \
encoder_deflate.c

libplugin_deflate_la_LDFLAGS = $(module_ldflags)
libplugin_deflate_la_SOURCES = $(encoder_deflate)

if STATIC_ENCODER_DEFLATE
static_encoder_deflate_src = $(encoder_deflate)
else
dynamic_encoder_deflate_lib = libplugin_deflate.la
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
# Validator LDAP
#
if HAVE_LDAP
validator_ldap = \
validator_ldap.c \
validator_ldap.h

libplugin_ldap_la_LDFLAGS = $(module_ldflags) 
libplugin_ldap_la_SOURCES = $(validator_ldap)
libplugin_ldap_la_LIBADD  = -lldap

if STATIC_VALIDATOR_LDAP
static_validator_ldap_src = $(validator_ldap)
static_validator_ldap_lib = -lldap
else
dynamic_validator_ldap_lib = libplugin_ldap.la
endif
endif


#
# Validator htpasswd
#
if USE_CRYPT
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
# Validator mysql
#
validator_mysql = \
validator_mysql.c \
validator_mysql.h

libplugin_mysql_la_LDFLAGS = $(module_ldflags) $(MYSQL_LDFLAGS)
libplugin_mysql_la_SOURCES = $(validator_mysql)
libplugin_mysql_la_CFLAGS  = $(MYSQL_CFLAGS)

if HAVE_MYSQL
if STATIC_VALIDATOR_MYSQL
static_validator_mysql_src   = $(validator_mysql)
static_validator_mysql_lib   = $(MYSQL_LDFLAGS)
static_validator_mysql_flags = $(MYSQL_CFLAGS)
else
dynamic_validator_mysql_lib = libplugin_mysql.la
endif
endif


#
# Balancer Round Robin
#
balancer_round_robin = \
balancer_round_robin.c \
balancer_round_robin.h

libplugin_round_robin_la_LDFLAGS = $(module_ldflags) 
libplugin_round_robin_la_SOURCES = $(balancer_round_robin)

if STATIC_BALANCER_ROUND_ROBIN
static_balancer_round_robin_src = $(balancer_round_robin)
else
dynamic_balancer_round_robin_lib = libplugin_round_robin.la
endif



lib_LTLIBRARIES = \
libcherokee-base.la \
libcherokee-client.la \
libcherokee-server.la \
libcherokee-config.la


# the macro puts -pthread into cflags, but we want to link
# with this flag too, so stick it into the ldflags if it's there...
# extra cflags at link time can't hurt
libcherokee_base_la_LDFLAGS = $(common_ldflags) $(library_soname)
libcherokee_base_la_LIBADD = \
$(TLS_LIBS) \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS) \
$(external_pcre_lib)

libcherokee_client_la_LDFLAGS = $(common_ldflags) $(library_soname)
libcherokee_client_la_LIBADD = \
$(TLS_LIBS) \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS) \
libcherokee-base.la

libcherokee_server_la_LDFLAGS = $(common_ldflags) $(library_soname)
libcherokee_server_la_CFLAGS = \
$(static_validator_htpasswd_flags) \
$(static_validator_mysql_flags) \
$(static_handler_dbslayer_flags)

libcherokee_server_la_LIBADD = \
$(TLS_LIBS) \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS) \
$(static_validator_pam_lib) \
$(static_validator_ldap_lib)  \
$(static_rule_geoip_lib)  \
$(static_validator_htpasswd_lib) \
$(static_validator_mysql_lib) \
$(static_handler_proxy_lib) \
$(static_handler_dbslayer_lib) \
libcherokee-base.la

libcherokee_config_la_LDFLAGS = $(common_ldflags) $(library_soname)
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
win32_misc.c \
win32_cservice.h \
win32_cservice.c
endif

libcherokee_base_la_SOURCES = \
$(zlib_src) \
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
init.h \
init.c \
avl.h \
avl.c \
avl_r.h \
avl_r.c \
http.h \
http.c \
list.h \
list.c \
ncpus.h \
ncpus.c \
cacheline.h \
cacheline.c \
crc32.h \
crc32.c \
macros.h \
common.h \
common-internal.h \
util.h \
util.c \
human_strcmp.h \
human_strcmp.c \
trace.h \
trace.c \
version.h \
version.c \
buffer.h \
buffer.c \
socket.h \
socket.c \
fdpoll.h \
fdpoll-protected.h \
fdpoll.c \
mime.h \
mime-protected.h \
mime.c \
mime_entry.c \
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
dtm.h \
dtm.c \
header.h \
header-protected.h \
header.c \
access.h \
access.c \
regex.h \
regex.c \
resolv_cache.h \
resolv_cache.c \
post.h \
post.c \
config_node.h \
config_node.c \
config_reader.h \
config_reader.c \
bogotime.h \
bogotime.c \
cache.h \
cache.c

libcherokee_client_la_SOURCES = \
url.h \
url.c \
request.h \
request.c \
downloader.h \
downloader-protected.h \
downloader.c \
downloader_async.h \
downloader_async.c


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
$(static_rule_directory_src) \
$(static_rule_extensions_src) \
$(static_rule_request_src) \
$(static_rule_header_src) \
$(static_rule_geoip_src) \
$(static_rule_not_src) \
$(static_rule_and_src) \
$(static_rule_or_src) \
\
$(static_handler_file_src) \
$(static_handler_admin_src) \
$(static_handler_dirlist_src) \
$(static_handler_scgi_src) \
$(static_handler_fcgi_src) \
$(static_handler_fastcgi_src) \
$(static_handler_redir_src) \
$(static_handler_error_redir_src) \
$(static_handler_error_nn_src) \
$(static_handler_cgi_src) \
$(static_handler_phpcgi_src) \
$(static_handler_common_src) \
$(static_handler_proxy_src) \
$(static_handler_mirror_src) \
$(static_handler_custom_error_src) \
$(static_handler_server_info_src) \
$(static_handler_dbslayer_src) \
\
$(static_logger_w3c_src) \
$(static_logger_ncsa_src) \
$(static_logger_combined_src) \
\
$(static_encoder_gzip_src) \
$(static_encoder_deflate_src) \
\
$(static_validator_pam_src) \
$(static_validator_ldap_src) \
$(static_validator_plain_src) \
$(static_validator_htdigest_src) \
$(static_validator_htpasswd_src) \
$(static_validator_mysql_src) \
\
$(static_balancer_round_robin_src) \
\
$(common_cgi) \
\
connection.h \
connection-protected.h \
connection.c \
handler.h \
handler.c \
rule.h \
rule.c \
rule_list.h \
rule_list.c \
rule_default.h \
rule_default.c \
config_entry.h \
config_entry.c \
server-protected.h \
server.h \
server.c \
virtual_server_names.h \
virtual_server_names.c \
virtual_server.h \
virtual_server.c \
encoder.h \
encoder.c \
logger.h \
logger.c \
logger_writer.h \
logger_writer.c \
thread.h \
thread.c \
plugin.h \
module.h \
module.c \
validator.h \
validator.c \
plugin_loader.h \
plugin_loader.c \
icons.h \
icons.c \
handler_error.c \
handler_error.h \
nonce.h \
nonce.c \
balancer.h \
balancer.c \
source.h \
source.c \
source_interpreter.h \
source_interpreter.c \
info.h \
info.c

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
\
macros.h \
common.h \
init.h \
util.h \
version.h \
buffer.h\
fdpoll.h \
http.h \
list.h \
mime_entry.h \
mime.h \
url.h \
header.h \
resolv_cache.h \
post.h \
trace.h \
avl.h \
cache.h \
\
config_node.h \
config_entry.h \
server.h \
module.h \
logger.h \
handler.h \
encoder.h \
connection.h \
plugin.h \
plugin_loader.h \
nonce.h \
rule.h \
validator.h \
\
downloader.h \
\
admin_client.h \
connection_info.h \
\
avl.h \
avl_r.h


#
# Cherokee plugins
#
plugindir = "$(libdir)/cherokee"
plugin_LTLIBRARIES = \
$(dynamic_rule_directory_lib) \
$(dynamic_rule_extensions_lib) \
$(dynamic_rule_request_lib) \
$(dynamic_rule_header_lib) \
$(dynamic_rule_geoip_lib) \
$(dynamic_rule_not_lib) \
$(dynamic_rule_and_lib) \
$(dynamic_rule_or_lib) \
$(dynamic_handler_file_lib) \
$(dynamic_handler_cgi_lib) \
$(dynamic_handler_phpcgi_lib) \
$(dynamic_handler_server_info_lib) \
$(dynamic_handler_admin_lib) \
$(dynamic_handler_dirlist_lib) \
$(dynamic_handler_scgi_lib) \
$(dynamic_handler_fcgi_lib) \
$(dynamic_handler_fastcgi_lib) \
$(dynamic_handler_redir_lib) \
$(dynamic_handler_proxy_lib) \
$(dynamic_handler_mirror_lib) \
$(dynamic_handler_custom_error_lib) \
$(dynamic_handler_error_redir_lib) \
$(dynamic_handler_error_nn_lib) \
$(dynamic_handler_common_lib) \
$(dynamic_handler_dbslayer_lib) \
$(dynamic_encoder_gzip_lib) \
$(dynamic_encoder_deflate_lib) \
$(dynamic_logger_ncsa_lib) \
$(dynamic_logger_combined_lib) \
$(dynamic_logger_w3c_lib) \
$(dynamic_validator_pam_lib) \
$(dynamic_validator_ldap_lib) \
$(dynamic_validator_plain_lib) \
$(dynamic_validator_htdigest_lib) \
$(dynamic_validator_htpasswd_lib) \
$(dynamic_validator_mysql_lib) \
$(dynamic_balancer_round_robin_lib) 


#
# Cherokee server worker
#
sbin_PROGRAMS = cherokee-worker cherokee-admin

if !PLATFORM_WIN32
sbin_PROGRAMS += cherokee
endif

cherokee_worker_SOURCES = main_worker.c
cherokee_worker_LDADD   = \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS) \
libcherokee-base.la \
libcherokee-server.la
cherokee_LDFLAGS = -export-dynamic

#
# Cherokee
#
cherokee_SOURCES = main.c

#
# Cherokee admin
#
cherokee_admin_SOURCES = main_admin.c
cherokee_admin_LDADD   = $(cherokee_worker_LDADD)
cherokee_admin_LDFLAGS = $(cherokee_worker_LDFLAGS)


#
# Log rotate utility
#
bin_PROGRAMS = cherokee-tweak

cherokee_tweak_SOURCES = main_tweak.c
cherokee_tweak_LDADD = \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS) \
libcherokee-base.la \
libcherokee-client.la \
libcherokee-server.la \
libcherokee-config.la



#
# Win32 service
#
if PLATFORM_WIN32
win32_cherokeeserv = win32_cherokeeserv
else
win32_cherokeeserv =
endif

win32_cherokeeserv_SOURCES = win32_cherokeeserv.c
win32_cherokeeserv_LDADD = libcherokee-config.la libcherokee-base.la libcherokee-client.la 

#
# Test programs
#
noinst_PROGRAMS = $(win32_cherokeeserv)

# test_SOURCES = test.c
# test_LDADD = libcherokee-config.la libcherokee-base.la libcherokee-client.la 


CLEANFILES = \
config.h

WINDOWS_PORT_FILES = \
Makefile.MingW \
Makefile.MingW.in \
unix4win32.h \
unix4win32.c \
win32_misc.h \
win32_misc.c

EXTRA_DIST = \
pcre/LICENCE \
$(pcre_src) \
$(win32_src) \
$(getopt_src) \
$(bin_SCRIPTS) \
$(validator_mysql) \
$(handler_dbslayer) \
$(cherokeedeps_DATA) \
$(cherokeedata_DATA) \
$(WINDOWS_PORT_FILES) \
loader.autoconf.h \
loader.autoconf.inc

distclean-local:
	$(RM) -f loader.autoconf.h loader.autoconf.inc
