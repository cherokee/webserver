SUBDIRS = develop

docdir = $(datadir)/doc/cherokee/
doc_DATA = \
index.html \
Authentication.html \
Authors.html \
Behavior_configuration.html \
CGI_executing.html \
Cget.html \
Combined_logger.html \
Combined_logs.html \
Common_behavior.html \
Comparison_servers.html \
Compiling_and_Installing.html \
Configuration.html \
Copying_and_license.html \
Current_events.html \
Directory_configuration.html \
Directory_listing.html \
Encoders.html \
Extension_configuration.html \
FastCGI.html \
File_sending.html \
Frequently_asked_questions.html \
Gzip_encoder.html \
How_to_generate_SSL_keys.html \
How_to_manage_Virtual_Host.html \
Htdigest_validator.html \
Htpasswd_validator.html \
Icons_configuration.html \
Jaws_and_Cherokee_with_FastCGI.html \
Logging_system.html \
Mime_types_configuration.html \
NCSA_logger.html \
NCSA_logs.html \
PAM_validator.html \
PHP_execution.html \
Plain_validator.html \
Redirections.html \
Request_configuration.html \
Ruby_on_Rails_with_SCGI.html \
SCGI.html \
Server_configuration.html \
U-Cherokee.html \
Validators.html \
W3C_logger.html \
W3C_logs.html \
WebApplications.html \
cherokee.png \
images/Cherokee-icono.png \
images/Exquisite-khelpcenter.png \
images/Server-conf-small.png 

EXTRA_DIST=\
$(doc_DATA)

index.html:
	wget -O cherokee_doc.tar.gz http://www.0x50.org/docs/cherokee_doc.tar.gz ;\
	gunzip -dc cherokee_doc.tar.gz | tar xfv - ;\
	rm -f cherokee_doc.tar.gz

