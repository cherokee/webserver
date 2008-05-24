docdir = $(datadir)/doc/cherokee/
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
How_to_generate_SSL_keys.html \
icons.html

dochandlersdir = $(docdir)handlers/
dochandlers_DATA = \
handlers/file.html \
handlers/dirlist.html \
handlers/common.html \
handlers/redir.html \
handlers/cgi.html \
handlers/fastcgi.html \
handlers/scgi.html

docloggersdir = $(docdir)loggers/
docloggers_DATA = \
loggers/combined.html \
loggers/ncsa.html \
loggers/w3c.html

docencodersdir = $(docdir)encoders/
docencoders_DATA = \
encoders/gzip.html

docbalancersdir = $(docdir)balancers/
docbalancers_DATA = \
balancers/round_robin.html

docvalidatorsdir = $(docdir)validators/
docvalidators_DATA = \
validators/htdigest.html \
validators/htpasswd.html \
validators/ldap.html \
validators/mysql.html \
validators/pam.html \
validators/plain.html

docmediacssdir = $(docdir)media/css/
docmediacss_DATA = \
media/css/documentation.css

docmediaimagesdir = $(docdir)media/images/
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

.NOTPARALLEL: $(GENERATED_FILES)
$(GENERATED_FILES):
	python $(top_srcdir)/doc/build_docs.py -v
