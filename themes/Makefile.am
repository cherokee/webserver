## Cherokee: methods/Makefile.am  -*- makefile -*-

cherokeethemesdir = "$(prefix)/share/cherokee/themes/"

THEMES = \
default_header.html  \
default_footer.html  \
default_entry.html   \
default_theme.css    \
default_logo.png     \
firefox3_header.html \
firefox3_footer.html \
firefox3_entry.html  \
firefox3_theme.css

install-data-local: $(THEMES)
	@$(mkinstalldirs) $(DESTDIR)$(cherokeethemesdir)/default
	$(INSTALL_DATA) $(top_srcdir)/themes/default_header.html $(DESTDIR)$(cherokeethemesdir)/default/header.html
	$(INSTALL_DATA) $(top_srcdir)/themes/default_footer.html $(DESTDIR)$(cherokeethemesdir)/default/footer.html
	$(INSTALL_DATA) $(top_srcdir)/themes/default_entry.html  $(DESTDIR)$(cherokeethemesdir)/default/entry.html
	$(INSTALL_DATA) $(top_srcdir)/themes/default_theme.css   $(DESTDIR)$(cherokeethemesdir)/default/theme.css
	$(INSTALL_DATA) $(top_srcdir)/themes/default_logo.png    $(DESTDIR)$(cherokeethemesdir)/default/cherokee-logo.png

	@$(mkinstalldirs) $(DESTDIR)$(cherokeethemesdir)/firefox3
	$(INSTALL_DATA) $(top_srcdir)/themes/firefox3_header.html $(DESTDIR)$(cherokeethemesdir)/firefox3/header.html
	$(INSTALL_DATA) $(top_srcdir)/themes/firefox3_footer.html $(DESTDIR)$(cherokeethemesdir)/firefox3/footer.html
	$(INSTALL_DATA) $(top_srcdir)/themes/firefox3_entry.html  $(DESTDIR)$(cherokeethemesdir)/firefox3/entry.html
	$(INSTALL_DATA) $(top_srcdir)/themes/firefox3_theme.css   $(DESTDIR)$(cherokeethemesdir)/firefox3/theme.css

uninstall-local: 
	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/default/header.html
	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/default/footer.html
	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/default/entry.html
	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/default/theme.css
	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/default/cherokee-logo.png

	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/firefox3/header.html
	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/firefox3/footer.html
	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/firefox3/entry.html
	$(RM) -f $(DESTDIR)$(cherokeethemesdir)/firefox3/theme.css


EXTRA_DIST = \
$(THEMES)
