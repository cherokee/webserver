## Cherokee: Makefile.am  -*- makefile -*-

cherokeewwwdir = "@WWW_ROOT@"

cherokeewwwimagesdir = "@WWW_ROOT@/images"
cherokeewwwimages_DATA = \
default-bg.png \
cherokee-logo.png \
powered_by_cherokee.png

HTMLS = \
index.html

install-data-local: $(HTMLS)
	@$(mkinstalldirs) $(DESTDIR)$(cherokeewwwdir)
	@if test -f $(DESTDIR)$(cherokeewwwdir)/index.html ; then \
		echo "$@ will not overwrite existing $(cherokeewwwdir)/index.html"; \
	else \
		$(INSTALL_DATA) $(top_srcdir)/www/index.html $(DESTDIR)$(cherokeewwwdir)/index.html; \
	fi

uninstall-local: 
	@if test -f $(DESTDIR)$(cherokeewwwdir)/index.html ; then \
		echo "rm -f $(DESTDIR)$(cherokeewwwdir)/index.html"; \
		$(RM) -f $(DESTDIR)$(cherokeewwwdir)/index.html; \
	fi


EXTRA_DIST = \
$(HTMLS) \
$(cherokeewwwimages_DATA) 
