docdir = $(datadir)/doc/cherokee/
doc_DATA = \
index.html \
authentication.html \
configuration.html \
loggers.html \
server.html \
vserver.html

dochandlersdir = $(docdir)handlers/
dochandlers_DATA = \
handlers/common.html \
handlers/dirlist.html \
handlers/file.html \
handlers/redir.html

docloggersdir = $(docdir)loggers/
docloggers_DATA = \
loggers/combined.html \
loggers/ncsa.html \
loggers/w3c.html

docvalidatorsdir = $(docdir)validators/
docvalidators_DATA = \
validators/mysql.html

docmediacssdir = $(docdir)media/css/
docmediacss_DATA = \
media/css/documentation.css

GENERATED_FILES = \
$(doc_DATA) \
$(dochandlers_DATA) \
$(docloggers_DATA) \
$(docvalidators_DATA)

EXTRA_DIST = \
$(GENERATED_FILES) \
$(docmediacss_DATA)

CLEANFILES = \
$(GENERATED_FILES) 

$(GENERATED_FILES):
	python build_docs.py
