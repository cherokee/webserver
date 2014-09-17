## Cherokee: cherokee/Makefile.am  -*- makefile -*-

AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = \
-I${top_srcdir} \
$(PTHREAD_CFLAGS)

#
# Scripts
#
bin_SCRIPTS = \
cherokee-panic \
cherokee-tweak \
cherokee-macos-askpass \
cherokee-admin-launcher

#
# Data
#
cherokeedepsdir = "$(datadir)/cherokee/deps"
cherokeedeps_DATA = \
common.deps \
combined.deps \
error_redir.deps \
scgi.deps \
fcgi.deps \
uwsgi.deps \
cgi.deps \
streaming.deps \
secdownload.deps \
render_rrd.deps \
post_report.deps

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
# File generation rules
#
BUILT_SOURCES = \
errors.h \
errors_defs.h

errors.h: errors.py error_list.py
	$(PYTHON) errors.py --errors $@

errors_defs.h: errors.py error_list.py
	$(PYTHON) errors.py --defines $@


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
# Rule exists
#
rule_exists = \
rule_exists.c \
rule_exists.h

libplugin_exists_la_LDFLAGS = $(module_ldflags)
libplugin_exists_la_SOURCES = $(rule_exists)

if STATIC_RULE_EXISTS
static_rule_exists_src = $(rule_exists)
else
dynamic_rule_exists_lib = libplugin_exists.la
endif


#
# Rule url_arg
#
rule_url_arg = \
rule_url_arg.c \
rule_url_arg.h

libplugin_url_arg_la_LDFLAGS = $(module_ldflags)
libplugin_url_arg_la_SOURCES = $(rule_url_arg)

if STATIC_RULE_URL_ARG
static_rule_url_arg_src = $(rule_url_arg)
else
dynamic_rule_url_arg_lib = libplugin_url_arg.la
endif


#
# Rule fullpath
#
rule_fullpath = \
rule_fullpath.c \
rule_fullpath.h

libplugin_fullpath_la_LDFLAGS = $(module_ldflags)
libplugin_fullpath_la_SOURCES = $(rule_fullpath)

if STATIC_RULE_FULL_PATH
static_rule_fullpath_src = $(rule_fullpath)
else
dynamic_rule_fullpath_lib = libplugin_fullpath.la
endif


#
# Rule bind
#
rule_bind = \
rule_bind.c \
rule_bind.h

libplugin_bind_la_LDFLAGS = $(module_ldflags)
libplugin_bind_la_SOURCES = $(rule_bind)

if STATIC_RULE_BIND
static_rule_bind_src = $(rule_bind)
else
dynamic_rule_bind_lib = libplugin_bind.la
endif


#
# Rule TLS
#
rule_tls = \
rule_tls.c \
rule_tls.h

libplugin_tls_la_LDFLAGS = $(module_ldflags)
libplugin_tls_la_SOURCES = $(rule_tls)

if STATIC_RULE_TLS
static_rule_tls_src = $(rule_tls)
else
dynamic_rule_tls_lib = libplugin_tls.la
endif


#
# Rule method
#
rule_method = \
rule_method.c \
rule_method.h

libplugin_method_la_LDFLAGS = $(module_ldflags)
libplugin_method_la_SOURCES = $(rule_method)

if STATIC_RULE_METHOD
static_rule_method_src = $(rule_method)
else
dynamic_rule_method_lib = libplugin_method.la
endif


#
# Rule from
#
rule_from = \
rule_from.c \
rule_from.h

libplugin_from_la_LDFLAGS = $(module_ldflags)
libplugin_from_la_SOURCES = $(rule_from)

if STATIC_RULE_FROM
static_rule_from_src = $(rule_from)
else
dynamic_rule_from_lib = libplugin_from.la
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
# VRule or
#
vrule_or = \
vrule_or.c \
vrule_or.h

libplugin_v_or_la_LDFLAGS = $(module_ldflags)
libplugin_v_or_la_SOURCES = $(vrule_or)

if STATIC_VRULE_OR
static_vrule_or_src = $(vrule_or)
else
dynamic_vrule_or_lib = libplugin_v_or.la
endif


#
# VRule wildcard
#
vrule_wildcard = \
vrule_wildcard.c \
vrule_wildcard.h

libplugin_wildcard_la_LDFLAGS = $(module_ldflags)
libplugin_wildcard_la_SOURCES = $(vrule_wildcard)

if STATIC_VRULE_WILDCARD
static_vrule_wildcard_src = $(vrule_wildcard)
else
dynamic_vrule_wildcard_lib = libplugin_wildcard.la
endif


#
# VRule rehost
#
vrule_rehost = \
vrule_rehost.c \
vrule_rehost.h

libplugin_rehost_la_LDFLAGS = $(module_ldflags)
libplugin_rehost_la_SOURCES = $(vrule_rehost)

if STATIC_VRULE_REHOST
static_vrule_rehost_src = $(vrule_rehost)
else
dynamic_vrule_rehost_lib = libplugin_rehost.la
endif


#
# VRule Target IP
#
vrule_target_ip = \
vrule_target_ip.c \
vrule_target_ip.h

libplugin_target_ip_la_LDFLAGS = $(module_ldflags)
libplugin_target_ip_la_SOURCES = $(vrule_target_ip)

if STATIC_VRULE_TARGET_IP
static_vrule_target_ip_src = $(vrule_target_ip)
else
dynamic_vrule_target_ip_lib = libplugin_target_ip.la
endif


#
# Collector RRD
#
if !STATIC_COLLECTOR_RRD
rrd_common = rrd_tools.h rrd_tools.c
endif

collector_rrd = \
$(rrd_common) \
collector_rrd.c \
collector_rrd.h

libplugin_rrd_la_LDFLAGS = $(module_ldflags)
libplugin_rrd_la_SOURCES = $(collector_rrd)

if STATIC_COLLECTOR_RRD
static_collector_rrd_src = $(collector_rrd)
else
dynamic_collector_rrd_lib = libplugin_rrd.la
endif


#
# Enhanced virtual hosts
#
gen_evhost = \
gen_evhost.c \
gen_evhost.h

libplugin_evhost_la_LDFLAGS = $(module_ldflags)
libplugin_evhost_la_SOURCES = $(gen_evhost)

if STATIC_GEN_EVHOST
static_gen_evhost_src = $(gen_evhost)
else
dynamic_gen_evhost_lib = libplugin_evhost.la
endif


#
# POST tracker
#
post_track = \
post_track.h \
post_track.c

libplugin_post_track_la_LDFLAGS = $(module_ldflags)
libplugin_post_track_la_SOURCES = $(post_track)

if STATIC_POST_TRACK
static_post_track_src = $(post_track)
else
dynamic_post_track_lib = libplugin_post_track.la
endif


#
# POST reporter
#
handler_post_report = \
handler_post_report.h \
handler_post_report.c

libplugin_post_report_la_LDFLAGS = $(module_ldflags)
libplugin_post_report_la_SOURCES = $(handler_post_report)

if STATIC_POST_REPORT
static_handler_post_report_src = $(handler_post_report)
else
dynamic_handler_post_report_lib = libplugin_post_report.la
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
# Handler render_rrd
#
if !STATIC_HANDLER_RENDER_RRD
rrd_common = rrd_tools.h rrd_tools.c
endif

handler_render_rrd = \
$(rrd_common) \
handler_render_rrd.c \
handler_render_rrd.h \
logo.inc

libplugin_render_rrd_la_LDFLAGS = $(module_ldflags)
libplugin_render_rrd_la_SOURCES = $(handler_render_rrd) $(handler_file)

if STATIC_HANDLER_RENDER_RRD
static_handler_render_rrd_src = $(handler_render_rrd)
else
dynamic_handler_render_rrd_lib = libplugin_render_rrd.la
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
libplugin_cgi_la_SOURCES = $(handler_cgi) $(handler_file)

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
libplugin_scgi_la_SOURCES = $(handler_scgi) $(handler_file)

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
libplugin_fcgi_la_SOURCES = $(handler_fcgi) $(handler_file)

if STATIC_HANDLER_FCGI
static_handler_fcgi_src = $(handler_fcgi)
else
dynamic_handler_fcgi_lib = libplugin_fcgi.la
endif


#
# Handler uwsgi
#
if !STATIC_HANDLER_UWSGI
handleruwsgi_cgi = handler_cgi_base.h handler_cgi_base.c
endif

handler_uwsgi = \
$(handleruwsgi_cgi) \
handler_uwsgi.h \
handler_uwsgi.c

libplugin_uwsgi_la_LDFLAGS = $(module_ldflags)
libplugin_uwsgi_la_SOURCES = $(handler_uwsgi) $(handler_file)

if STATIC_HANDLER_UWSGI
static_handler_uwsgi_src = $(handler_uwsgi)
else
dynamic_handler_uwsgi_lib = libplugin_uwsgi.la
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

libplugin_error_redir_la_LDFLAGS = $(module_ldflags)
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

libplugin_common_la_LDFLAGS = $(module_ldflags)
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
handler_proxy.h \
proxy_hosts.h \
proxy_hosts.c

libplugin_proxy_la_LDFLAGS = $(module_ldflags)
libplugin_proxy_la_SOURCES = $(handler_proxy)

if STATIC_HANDLER_PROXY
static_handler_proxy_src = $(handler_proxy)
static_handler_proxy_lib = libcherokee-client.la
else
dynamic_handler_proxy_lib = libplugin_proxy.la
endif



#
# Handler ssi
#
handler_ssi = \
handler_ssi.c \
handler_ssi.h

libplugin_ssi_la_LDFLAGS = $(module_ldflags)
libplugin_ssi_la_SOURCES = $(handler_ssi)

if STATIC_HANDLER_SSI
static_handler_ssi_src = $(handler_ssi)
else
dynamic_handler_ssi_lib = libplugin_ssi.la
endif


#
# Handler secdownload
#
handler_secdownload = \
handler_secdownload.c \
handler_secdownload.h

libplugin_secdownload_la_LDFLAGS = $(module_ldflags)
libplugin_secdownload_la_SOURCES = $(handler_secdownload)

if STATIC_HANDLER_SECDOWNLOAD
static_handler_secdownload_src = $(handler_secdownload)
else
dynamic_handler_secdownload_lib = libplugin_secdownload.la
endif


#
# Handler empty_gif
#
handler_empty_gif = \
handler_empty_gif.c \
handler_empty_gif.h

libplugin_empty_gif_la_LDFLAGS = $(module_ldflags)
libplugin_empty_gif_la_SOURCES = $(handler_empty_gif)

if STATIC_HANDLER_EMPTY_GIF
static_handler_empty_gif_src = $(handler_empty_gif)
else
dynamic_handler_empty_gif_lib = libplugin_empty_gif.la
endif


#
# Handler drop
#
handler_drop = \
handler_drop.c \
handler_drop.h

libplugin_drop_la_LDFLAGS = $(module_ldflags)
libplugin_drop_la_SOURCES = $(handler_drop)

if STATIC_HANDLER_DROP
static_handler_drop_src = $(handler_drop)
else
dynamic_handler_drop_lib = libplugin_drop.la
endif



#
# Handler streaming
#
handler_streaming = \
handler_streaming.c \
handler_streaming.h

libplugin_streaming_la_LDFLAGS = $(module_ldflags)
libplugin_streaming_la_SOURCES = $(handler_streaming)
libplugin_streaming_la_LIBADD  = $(FFMPEG_LIBS)
libplugin_streaming_la_CFLAGS  = $(FFMPEG_CFLAGS)

if HAVE_FFMPEG
if STATIC_HANDLER_STREAMING
static_handler_streaming_src   = $(handler_streaming)
static_handler_streaming_lib   = $(FFMPEG_LIBS)
static_handler_streaming_flags = $(FFMPEG_CFLAGS)
else
dynamic_handler_streaming_lib = libplugin_streaming.la
endif
endif


#
# Handler DB Slayer
#
handler_dbslayer = \
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
handler_admin = \
handler_admin.c \
handler_admin.h \
admin_server.h \
admin_server.c

libplugin_admin_la_LDFLAGS = $(module_ldflags)
libplugin_admin_la_SOURCES = $(handler_admin)
libplugin_admin_la_LIBADD  = libcherokee-client.la

if STATIC_HANDLER_ADMIN
static_handler_admin_src = $(handler_admin)
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
# Logger Custom
#
logger_custom = \
logger_custom.h \
logger_custom.c

libplugin_custom_la_LDFLAGS = $(module_ldflags)
libplugin_custom_la_SOURCES = $(logger_custom)

if STATIC_LOGGER_CUSTOM
static_logger_custom_src = $(logger_custom)
else
dynamic_logger_custom_lib = libplugin_custom.la
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

libplugin_combined_la_LDFLAGS = $(module_ldflags)
libplugin_combined_la_SOURCES = $(logger_combined)

if STATIC_LOGGER_COMBINED
static_logger_combined_src = $(logger_combined)
else
dynamic_logger_combined_lib = libplugin_combined.la
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
if !STATIC_VALIDATOR_HTPASSWD
validator_file_file = validator_file.h validator_file.c
endif

if USE_CRYPT
validator_htpasswd = \
$(validator_file_file) \
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
if !STATIC_VALIDATOR_PLAIN
validator_file_file = validator_file.h validator_file.c
endif

validator_plain = \
$(validator_file_file) \
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
if !STATIC_VALIDATOR_HTDIGEST
validator_file_file = validator_file.h validator_file.c
endif

validator_htdigest =   \
$(validator_file_file) \
validator_htdigest.c   \
validator_htdigest.h

libplugin_htdigest_la_LDFLAGS = $(module_ldflags)
libplugin_htdigest_la_SOURCES = $(validator_htdigest)

if STATIC_VALIDATOR_HTDIGEST
static_validator_htdigest_src = $(validator_htdigest)
else
dynamic_validator_htdigest_lib = libplugin_htdigest.la
endif


#
# Validator authlist
#
validator_authlist = \
validator_authlist.c \
validator_authlist.h

libplugin_authlist_la_LDFLAGS = $(module_ldflags)
libplugin_authlist_la_SOURCES = $(validator_authlist)

if STATIC_VALIDATOR_AUTHLIST
static_validator_authlist_src = $(validator_authlist)
else
dynamic_validator_authlist_lib = libplugin_authlist.la
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
# Cryptor libssl
#
cryptor_libssl = \
cryptor_libssl.c \
cryptor_libssl.h

cryptor_libssl_dist = \
cryptor_libssl_dh_512.c \
cryptor_libssl_dh_1024.c \
cryptor_libssl_dh_2048.c \
cryptor_libssl_dh_4096.c

libplugin_libssl_la_LDFLAGS = $(module_ldflags) $(LIBSSL_LIBS)
libplugin_libssl_la_SOURCES = $(cryptor_libssl)
libplugin_libssl_la_CFLAGS  = $(LIBSSL_CFLAGS)

if USE_OPENSSL
if STATIC_CRYPTOR_LIBSSL
static_cryptor_libssl_src   = $(cryptor_libssl)
static_cryptor_libssl_lib   = $(LIBSSL_LIBS)
static_cryptor_libssl_flags = $(LIBSSL_CFLAGS)
else
dynamic_cryptor_libssl_lib = libplugin_libssl.la
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


#
# Balancer IP Hash
#
balancer_ip_hash = \
balancer_ip_hash.c \
balancer_ip_hash.h

libplugin_ip_hash_la_LDFLAGS = $(module_ldflags)
libplugin_ip_hash_la_SOURCES = $(balancer_ip_hash)

if STATIC_BALANCER_IP_HASH
static_balancer_ip_hash_src = $(balancer_ip_hash)
else
dynamic_balancer_ip_hash_lib = libplugin_ip_hash.la
endif


#
# Balancer Failover
#
balancer_failover = \
balancer_failover.c \
balancer_failover.h

libplugin_failover_la_LDFLAGS = $(module_ldflags)
libplugin_failover_la_SOURCES = $(balancer_failover)

if STATIC_BALANCER_FAILOVER
static_balancer_failover_src = $(balancer_failover)
else
dynamic_balancer_failover_lib = libplugin_failover.la
endif


lib_LTLIBRARIES = \
libcherokee-base.la \
libcherokee-client.la \
libcherokee-server.la


# the macro puts -pthread into cflags, but we want to link
# with this flag too, so stick it into the ldflags if it's there...
# extra cflags at link time can't hurt
libcherokee_base_la_LDFLAGS = $(common_ldflags) $(library_soname)
libcherokee_base_la_LIBADD = \
$(RT_LIBS) \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS) \
$(external_pcre_lib)

libcherokee_client_la_LDFLAGS = $(common_ldflags) $(library_soname)
libcherokee_client_la_LIBADD = \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS) \
libcherokee-base.la

libcherokee_server_la_LDFLAGS = $(common_ldflags) $(library_soname)
libcherokee_server_la_CFLAGS = \
$(static_validator_htpasswd_flags) \
$(static_validator_mysql_flags) \
$(static_cryptor_libssl_flags) \
$(static_handler_dbslayer_flags) \
$(static_handler_streaming_flags)

libcherokee_server_la_LIBADD = \
$(PTHREAD_LIBS) \
$(PTHREAD_CFLAGS) \
$(static_validator_pam_lib) \
$(static_validator_ldap_lib) \
$(static_rule_geoip_lib) \
$(static_validator_htpasswd_lib) \
$(static_validator_mysql_lib) \
$(static_handler_proxy_lib) \
$(static_handler_dbslayer_lib) \
$(static_handler_streaming_lib) \
$(static_cryptor_libssl_lib) \
libcherokee-base.la


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


libcherokee_base_la_SOURCES = \
$(zlib_src) \
$(internal_pcre_src) \
$(internal_getopt_src) \
$(poll_poll_src) \
$(poll_epoll_src) \
$(poll_kqueue_src) \
$(poll_port_src) \
$(poll_select_src) \
cherokee.h \
init.h \
init.c \
threading.h \
threading.c \
avl_generic.h \
avl_generic.c \
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
nullable.h \
util.h \
util.c \
error_log.h \
error_log.c \
errors.h \
errors_defs.h \
human_strcmp.h \
human_strcmp.c \
trace.h \
trace.c \
version.h \
version.c \
buffer.h \
buffer.c \
template.h \
template.c \
module.h \
module.c \
cryptor.h \
cryptor.c \
socket_lowlevel.h \
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
sha512.h \
sha512.c \
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
cache.c \
logger.h \
logger.c \
logger_writer.h \
logger_writer.c \
xrealip.h \
xrealip.c \
shm.h \
shm.c


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
if STATIC_HANDLER_CGI
   common_cgi = handler_cgi_base.h \
	     	 handler_cgi_base.c
endif
if STATIC_HANDLER_SCGI
   common_cgi = handler_cgi_base.h \
	     	 handler_cgi_base.c
endif
if STATIC_HANDLER_UWSGI
   common_cgi = handler_cgi_base.h \
	     	 handler_cgi_base.c
endif

if STATIC_VALIDATOR_PLAIN
   common_val_file = validator_file.h \
				 validator_file.c
endif

if STATIC_VALIDATOR_HTPASSWD
   common_val_file = validator_file.h \
				 validator_file.c
endif

if STATIC_VALIDATOR_HTDIGEST
   common_val_file = validator_file.h \
				 validator_file.c
endif

if STATIC_COLLECTOR_RRD
   common_rrd_tools = rrd_tools.h \
				  rrd_tools.c
endif

if STATIC_HANDLER_RENDER_RRD
   common_rrd_tools = rrd_tools.h \
				  rrd_tools.c
endif

libcherokee_server_la_SOURCES = \
$(static_rule_directory_src) \
$(static_rule_extensions_src) \
$(static_rule_request_src) \
$(static_rule_header_src) \
$(static_rule_exists_src) \
$(static_rule_url_arg_src) \
$(static_rule_fullpath_src) \
$(static_rule_bind_src) \
$(static_rule_tls_src) \
$(static_rule_method_src) \
$(static_rule_from_src) \
$(static_rule_geoip_src) \
$(static_rule_not_src) \
$(static_rule_and_src) \
$(static_rule_or_src) \
\
$(static_vrule_or_src) \
$(static_vrule_wildcard_src) \
$(static_vrule_rehost_src) \
$(static_vrule_target_ip_src) \
\
$(static_gen_evhost_src) \
\
$(static_post_track_src) \
$(static_handler_post_report_src) \
\
$(static_collector_rrd_src) \
\
$(static_handler_file_src) \
$(static_handler_dirlist_src) \
$(static_handler_cgi_src) \
$(static_handler_fcgi_src) \
$(static_handler_scgi_src) \
$(static_handler_proxy_src) \
$(static_handler_uwsgi_src) \
$(static_handler_redir_src) \
$(static_handler_error_redir_src) \
$(static_handler_error_nn_src) \
$(static_handler_common_src) \
$(static_handler_ssi_src) \
$(static_handler_secdownload_src) \
$(static_handler_empty_gif_src) \
$(static_handler_drop_src) \
$(static_handler_streaming_src) \
$(static_handler_custom_error_src) \
$(static_handler_server_info_src) \
$(static_handler_render_rrd_src) \
$(static_handler_dbslayer_src) \
$(static_handler_admin_src) \
\
$(static_logger_ncsa_src) \
$(static_logger_custom_src) \
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
$(static_validator_authlist_src) \
$(static_validator_mysql_src) \
\
$(static_cryptor_libssl_src) \
\
$(static_balancer_round_robin_src) \
$(static_balancer_ip_hash_src) \
$(static_balancer_failover_src) \
\
$(common_cgi) \
$(common_val_file) \
$(common_rrd_tools) \
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
vrule.h \
vrule.c \
config_entry.h \
config_entry.c \
server-protected.h \
server.h \
server.c \
bind.h \
bind.c \
virtual_server.h \
virtual_server.c \
encoder.h \
encoder.c \
thread.h \
thread.c \
plugin.h \
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
info.c \
connection_info.h \
connection_info.c \
dwriter.h \
dwriter.c \
limiter.h \
limiter.c \
services.h \
services-common.c \
services-client.c \
collector.h \
collector.c \
header_op.h \
header_op.c \
flcache.h \
flcache.c \
avl_flcache.h \
avl_flcache.c


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
template.h\
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
cache.h \
iocache.h \
module.h \
cryptor.h \
nullable.h \
\
config_node.h \
config_entry.h \
server.h \
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
$(dynamic_rule_exists_lib) \
$(dynamic_rule_url_arg_lib) \
$(dynamic_rule_fullpath_lib) \
$(dynamic_rule_bind_lib) \
$(dynamic_rule_tls_lib) \
$(dynamic_rule_method_lib) \
$(dynamic_rule_from_lib) \
$(dynamic_rule_geoip_lib) \
$(dynamic_rule_not_lib) \
$(dynamic_rule_and_lib) \
$(dynamic_rule_or_lib) \
$(dynamic_vrule_or_lib) \
$(dynamic_vrule_wildcard_lib) \
$(dynamic_vrule_rehost_lib) \
$(dynamic_vrule_target_ip_lib) \
$(dynamic_collector_rrd_lib) \
$(dynamic_gen_evhost_lib) \
$(dynamic_post_track_lib) \
$(dynamic_handler_post_report_lib) \
$(dynamic_handler_file_lib) \
$(dynamic_handler_server_info_lib) \
$(dynamic_handler_render_rrd_lib) \
$(dynamic_handler_dirlist_lib) \
$(dynamic_handler_cgi_lib) \
$(dynamic_handler_fcgi_lib) \
$(dynamic_handler_scgi_lib) \
$(dynamic_handler_proxy_lib) \
$(dynamic_handler_uwsgi_lib) \
$(dynamic_handler_redir_lib) \
$(dynamic_handler_ssi_lib) \
$(dynamic_handler_secdownload_lib) \
$(dynamic_handler_empty_gif_lib) \
$(dynamic_handler_drop_lib) \
$(dynamic_handler_streaming_lib) \
$(dynamic_handler_custom_error_lib) \
$(dynamic_handler_error_redir_lib) \
$(dynamic_handler_error_nn_lib) \
$(dynamic_handler_common_lib) \
$(dynamic_handler_dbslayer_lib) \
$(dynamic_handler_admin_lib) \
$(dynamic_encoder_gzip_lib) \
$(dynamic_encoder_deflate_lib) \
$(dynamic_logger_ncsa_lib) \
$(dynamic_logger_custom_lib) \
$(dynamic_logger_combined_lib) \
$(dynamic_validator_pam_lib) \
$(dynamic_validator_ldap_lib) \
$(dynamic_validator_plain_lib) \
$(dynamic_validator_htdigest_lib) \
$(dynamic_validator_htpasswd_lib) \
$(dynamic_validator_authlist_lib) \
$(dynamic_validator_mysql_lib) \
$(dynamic_cryptor_libssl_lib) \
$(dynamic_balancer_round_robin_lib) \
$(dynamic_balancer_ip_hash_lib) \
$(dynamic_balancer_failover_lib)


#
# Cherokee server worker
#
sbin_PROGRAMS = cherokee-worker cherokee-admin cherokee

cherokee_worker_SOURCES = main_worker.c
cherokee_worker_LDFLAGS = -export-dynamic
cherokee_worker_LDADD   = \
$(libcherokee_server_la_LIBADD) \
libcherokee-base.la \
libcherokee-server.la

#
# Cherokee
#
cherokee_SOURCES = main.c services-common.c services-server.c
cherokee_LDADD   = $(PTHREAD_LIBS) $(RT_LIBS) libcherokee-base.la

#
# Cherokee admin
#
cherokee_admin_SOURCES = main_admin.c
cherokee_admin_LDADD   = $(cherokee_worker_LDADD)
cherokee_admin_LDFLAGS = $(cherokee_worker_LDFLAGS)

# test_SOURCES = test.c
# test_LDADD = libcherokee-base.la libcherokee-client.la


CLEANFILES = \
config.h

EXTRA_DIST = \
pcre/LICENCE \
errors.py \
error_list.py \
errors.h \
errors_defs.h \
$(pcre_src) \
$(getopt_src) \
$(bin_SCRIPTS) \
$(validator_mysql) \
$(handler_dbslayer) \
$(cherokeedeps_DATA) \
$(cryptor_libssl_dist)

distclean-local:
	$(RM) -f loader.autoconf.h loader.autoconf.inc errors.h errors_defs.h
