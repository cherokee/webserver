## Cherokee: methods/Makefile.am  -*- makefile -*-

cherokeethemesdir = "$(prefix)/share/cherokee/themes/"

THEMES = \
default_header.html \
default_footer.html \
default_entry.html  \
default_logo.png

install-data-local: $(THEMES)
	@$(mkinstalldirs) $(DESTDIR)$(cherokeethemesdir)/default
	$(INSTALL_DATA) $(top_srcdir)/themes/default_header.html $(DESTDIR)$(cherokeethemesdir)/default/header.html
	$(INSTALL_DATA) $(top_srcdir)/themes/default_footer.html $(DESTDIR)$(cherokeethemesdir)/default/footer.html
	$(INSTALL_DATA) $(top_srcdir)/themes/default_entry.html  $(DESTDIR)$(cherokeethemesdir)/default/entry.html
	$(INSTALL_DATA) $(top_srcdir)/themes/default_theme.css   $(DESTDIR)$(cherokeethemesdir)/default/theme.css
	$(INSTALL_DATA) $(top_srcdir)/themes/default_logo.png    $(DESTDIR)$(cherokeethemesdir)/default/cherokee-logo.png

uninstall-local: 
	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/default/header.html
	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/default/footer.html
	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/default/entry.html
	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/default/theme.css
	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/default/cherokee-logo.png

EXTRA_DIST = \
$(THEMES)
