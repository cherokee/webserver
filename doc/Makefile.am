
docdir = $(datadir)/doc/cherokee/
doc_DATA = \
output/index.html \
output/authentication.html \
output/configuration.html \
output/loggers.html \
output/server.html \
output/vserver.html

dochandlersdir = $(docdir)handlers/
dochandlers_DATA = \
output/handlers/common.html \
output/handlers/dirlist.html \
output/handlers/file.html \
output/handlers/redir.html

docloggersdir = $(docdir)loggers/
docloggers_DATA = \
output/loggers/combined.html \
output/loggers/ncsa.html \
output/loggers/w3c.html

docmediacssdir = $(docdir)media/css/
docmediacss_DATA = \
output/media/css/documentation.css

docvalidatorsdir = $(docdir)validators/
docvalidators_DATA = \
output/validators/mysql.html

EXTRA_DIST = \
$(doc_DATA) \
$(dochandlers_DATA) \
$(docloggers_DATA) \
$(docmediacss_DATA) \
$(docvalidators_DATA)

output/index.html:
	python build_docs.py
    