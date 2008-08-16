ASCIIDOC=python $(top_srcdir)/doc/build/asciidoc.py --unsafe --conf-file=$(top_srcdir)/doc/build/doc.conf

SUFFIXES = .txt .html

.txt.html:
	$(ASCIIDOC) -o $@ $(top_srcdir)/doc/$< 

docdir = $(datadir)/doc/cherokee
doc_DATA = \
index.html \
basics.html \
basics_why_cherokee.html \
basics_requirements.html \
basics_download.html \
basics_installation.html \
basics_installation_unix.html \
basics_installation_windows.html \
basics_installation_svn.html \
basics_upgrade.html \
basics_running_cherokee.html \
bundle.html \
bundle_cherokee.html \
bundle_cherokee-admin.html \
bundle_cherokee-config.html \
bundle_cherokee-guardian.html \
bundle_cherokee-tweak.html \
bundle_spawn-fcgi.html \
bundle_cget.html \
config.html \
config_quickstart.html \
config_general.html \
config_virtual_servers.html \
config_virtual_servers_rule.html \
config_encoding.html \
config_icons.html \
config_mime_types.html \
config_advanced.html \
cookbook.html \
cookbook_managing_logs.html \
cookbook_optimizations.html \
cookbook_cross_compilation.html \
cookbook_django.html \
cookbook_php.html \
cookbook_ror.html \
cookbook_authentication.html \
cookbook_ssl.html \
modules.html \
modules_balancers.html \
modules_balancers_round_robin.html \
modules_encoders.html \
modules_encoders_gzip.html \
modules_encoders_deflate.html \
modules_handlers.html \
modules_handlers_file.html \
modules_handlers_dirlist.html \
modules_handlers_common.html \
modules_handlers_redir.html \
modules_handlers_cgi.html \
modules_handlers_fcgi.html \
modules_handlers_scgi.html \
modules_handlers_server_info.html \
modules_handlers_mirror.html \
modules_handlers_admin.html \
modules_loggers.html \
modules_loggers_combined.html \
modules_loggers_ncsa.html \
modules_loggers_w3c.html \
modules_validators.html \
modules_validators_htdigest.html \
modules_validators_htpasswd.html \
modules_validators_ldap.html \
modules_validators_mysql.html \
modules_validators_pam.html \
modules_validators_plain.html \
other.html \
other_goodies.html \
dev.html \
dev_debug.html \
dev_cherokee.conf.html

docmediacssdir = $(docdir)/media/css/
docmediacss_DATA = \
media/css/cherokee_doc.css

docmediaimagesdir = $(docdir)/media/images/
docmediaimages_DATA = \
media/images/admin_advanced.png \
media/images/admin_behaviour.png \
media/images/admin_dirlist_cherokee.png \
media/images/admin_encoding_gzip.png \
media/images/admin_encoding.png \
media/images/admin_general.png \
media/images/admin_handler_admin.png \
media/images/admin_handler_cgi.png \
media/images/admin_handler_dirlist_ex.png \
media/images/admin_handler_dirlist.png \
media/images/admin_handler_fastcgi1.png \
media/images/admin_handler_fastcgi2.png \
media/images/admin_handler_file.png \
media/images/admin_handler_onlylisting_ex.png \
media/images/admin_handler_onlylisting.png \
media/images/admin_icons1.png \
media/images/admin_icons2.png \
media/images/admin_icons3.png \
media/images/admin_index.png \
media/images/admin_launch.png \
media/images/admin_mime.png \
media/images/admin_noconfig.png \
media/images/admin_notrunning.png \
media/images/admin_rule_regex.png \
media/images/admin_validators_pam.png \
media/images/admin_validators_plain.png \
media/images/admin_vserver_errors.png \
media/images/admin_vserver_loggers.png \
media/images/admin_vserver.png \
media/images/admin_vserver_vserver.png \
media/images/cookbook_django_common.png \
media/images/cookbook_django_scgi.png \
media/images/cookbook_ror_common.png \
media/images/cookbook_ror_fcgi.png \
media/images/cookbook_mysql_rule.png \
media/images/cookbook_mysql_validator.png \
media/images/tracelor.png

EXTRA_DIST = \
$(doc_DATA) \
$(docmediacss_DATA) \
$(docmediaimages_DATA)
