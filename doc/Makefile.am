ASCIIDOC=python $(top_srcdir)/doc/build/asciidoc.py -b xhtml11 --conf-file=$(top_srcdir)/doc/build/chunked_template.conf -a cherokee_version=$(PACKAGE_VERSION)

SUFFIXES = .txt .html

.txt.html:
	$(ASCIIDOC) -o $@ $< 

docdir = $(datadir)/doc/cherokee
doc_DATA = \
index.html \
authentication.html \
general.html \
loggers.html \
vserver.html \
debug.html \
entry.html \
compiling_and_installing_unix.html \
compiling_and_installing_windows.html \
cross_compile_cherokee.html \
internal_configuration.html \
advanced.html \
admin.html \
download.html \
generating_ssl_keys.html \
icons.html

dochandlersdir = $(docdir)/
dochandlers_DATA = \
modules_handlers_file.html \
modules_handlers_dirlist.html \
modules_handlers_common.html \
modules_handlers_redir.html \
modules_handlers_cgi.html \
modules_handlers_fastcgi.html \
modules_handlers_scgi.html

docloggersdir = $(docdir)/
docloggers_DATA = \
modules_loggers_combined.html \
modules_loggers_ncsa.html \
modules_loggers_w3c.html

docencodersdir = $(docdir)/
docencoders_DATA = \
modules_encoders_gzip.html

docbalancersdir = $(docdir)/
docbalancers_DATA = \
modules_balancers_round_robin.html

docvalidatorsdir = $(docdir)/
docvalidators_DATA = \
modules_validators_htdigest.html \
modules_validators_htpasswd.html \
modules_validators_ldap.html \
modules_validators_mysql.html \
modules_validators_pam.html \
modules_validators_plain.html

docmediacssdir = $(docdir)/media/css/
docmediacss_DATA = \
media/css/cherokee_doc.css

docmediaimagesdir = $(docdir)/media/images/
docmediaimages_DATA = \
media/images/admin_handler_fastcgi2.png \
media/images/admin_index.png \
media/images/admin_handler_fastcgi1.png \
media/images/admin_vserver.png \
media/images/admin_vserver_errors.png \
media/images/admin_icons3.png \
media/images/admin_handler_file.png \
media/images/admin_behaviour.png \
media/images/admin_launch.png \
media/images/admin_vserver_loggers.png \
media/images/admin_handler_cgi.png \
media/images/admin_icons2.png \
media/images/admin_general.png \
media/images/admin_icons1.png \
media/images/admin_encoding.png \
media/images/admin_validators_plain.png \
media/images/admin_validators_pam.png \
media/images/admin_advanced.png \
media/images/admin_handler_dirlist.png \
media/images/admin_handler_dirlist_ex.png \
media/images/admin_handler_onlylisting.png \
media/images/admin_handler_onlylisting_ex.png


GENERATED_FILES = \
$(doc_DATA) \
$(dochandlers_DATA) \
$(docloggers_DATA) \
$(docvalidators_DATA) \
$(docbalancers_DATA) \
$(docencoders_DATA)


EXTRA_DIST = \
$(GENERATED_FILES) \
$(docmediacss_DATA) \
$(docmediaimages_DATA)


CLEANFILES = \
$(GENERATED_FILES) 
