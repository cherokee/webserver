## Cherokee: Makefile.am  -*- makefile -*-

SUBDIRS = m4 contrib www doc icons themes qa cherokee cget windows
bin_SCRIPTS = cherokee-config
SUFFIXES = .sample.pre .sample

# M4 macro file for inclusion with autoconf
m4datadir = "$(datadir)/aclocal"
m4data_DATA = cherokee.m4

# Man pag
man_MANS = \
cget.1 \
cherokee.1 \
cherokee-config.1 \
cherokee_logrotate.1

# pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = cherokee.pc

# Replacement utility
noinst_PROGRAMS = cherokee_replace
cherokee_replace_SOURCES = cherokee_replace.c

# Configuration files
cherokeeconfdir = $(sysconfdir)/cherokee

.sample.pre.sample:
	$(top_builddir)/cherokee_replace $< $@ "%sysconfdir%" "${sysconfdir}" "%datadir%" "${datadir}" "%prefix%" "${prefix}" "%wwwroot%" "${WWW_ROOT}"

CONFS_PRE =              \
cherokee.conf.sample.pre \
performance.conf.sample.pre

CONFS_DONE =           \
mods-icons.conf.sample \
mime.types.sample      \
mime.compression.types.sample

CONFS = \
$(CONFS_PRE:.sample.pre=.sample)

WINDOWS_PORT_FILES = \
config.h.win32

DISTCLEANFILES = \
cherokee.pc      \
$(CONFS)

COMPILATION_TESTS = \
debian_testing.sh \
debian_testing_chroot.sh


EXTRA_DIST =          \
TODO                  \
http-cherokee.xml     \
cherokee.spec         \
cherokee.spec.in      \
SUNWcherokee.spec     \
pam.d_cherokee        \
cherokee.pc.in        \
$(m4data_DATA)        \
$(CONFS_DONE)         \
$(CONFS_PRE)          \
$(man_MANS)           \
$(WINDOWS_PORT_FILES) \
$(COMPILATION_TESTS)


install-data-local-config:
	@$(mkinstalldirs) $(DESTDIR)$(pkgincludedir);
	$(INSTALL_DATA) $(top_builddir)/config.h $(DESTDIR)$(pkgincludedir)/cherokee-config.h

uninstall-local-config:
	@if test -f $(DESTDIR)$(pkgincludedir)/cherokee-config.h; then \
		echo "rm -f $(DESTDIR)$(pkgincludedir)/cherokee-config.h"; \
		$(RM) -f $(DESTDIR)$(pkgincludedir)/cherokee-config.h; \
	fi


install-data-local: $(CONFS) $(CONFS_DONE) install-data-local-config
	@$(mkinstalldirs) $(DESTDIR)$(cherokeeconfdir)
	@$(mkinstalldirs) $(DESTDIR)$(cherokeeconfdir)/ssl
	@$(mkinstalldirs) $(DESTDIR)$(cherokeeconfdir)/mods-available
	@$(mkinstalldirs) $(DESTDIR)$(cherokeeconfdir)/mods-enabled
	@$(mkinstalldirs) $(DESTDIR)$(cherokeeconfdir)/sites-available
	@$(mkinstalldirs) $(DESTDIR)$(cherokeeconfdir)/sites-enabled
	@if test -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf ; then \
		echo "$@ will not overwrite existing $(DESTDIR)$(cherokeeconfdir)/cherokee.conf"; \
	else \
		$(INSTALL_DATA) $(top_builddir)/cherokee.conf.sample $(DESTDIR)$(cherokeeconfdir)/cherokee.conf; \
	fi
	@if test -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf.perf_sample ; then \
		echo "$@ will not overwrite existing $(DESTDIR)$(cherokeeconfdir)/cherokee.conf.perf_sample"; \
	else \
		$(INSTALL_DATA) $(top_builddir)/performance.conf.sample $(DESTDIR)$(cherokeeconfdir)/cherokee.conf.perf_sample; \
	fi
	@if test -f $(DESTDIR)$(cherokeeconfdir)/mime.types ; then \
		echo "$@ will not overwrite existing $(DESTDIR)$(cherokeeconfdir)/mime.types"; \
	else \
		$(INSTALL_DATA) $(top_srcdir)/mime.types.sample $(DESTDIR)$(cherokeeconfdir)/mime.types; \
	fi
	@if test -f $(DESTDIR)$(cherokeeconfdir)/mime.compression.types ; then \
		echo "$@ will not overwrite existing $(DESTDIR)$(cherokeeconfdir)/mime.compression.types"; \
	else \
		$(INSTALL_DATA) $(top_srcdir)/mime.compression.types.sample $(DESTDIR)$(cherokeeconfdir)/mime.compression.types; \
	fi
	@if test -d $(DESTDIR)$(sysconfdir)/pam.d && test ! -d $(DESTDIR)$(sysconfdir)/pam.d/cherokee ; then \
		$(INSTALL_DATA) $(top_srcdir)/pam.d_cherokee $(DESTDIR)$(sysconfdir)/pam.d/cherokee; \
	fi
	@if test -f $(DESTDIR)$(cherokeeconfdir)/mods-available/icons ; then \
		echo "$@ will not overwrite existing $(DESTDIR)$(cherokeeconfdir)/mods-available/icons"; \
	else \
		$(INSTALL_DATA) $(top_builddir)/icons.conf.sample $(DESTDIR)$(cherokeeconfdir)/mods-available/icons; \
		$(SHELL) -c "cd $(DESTDIR)$(cherokeeconfdir)/mods-enabled/ && $(LN_S) ../mods-available/icons ."; \
	fi
	@if test -f $(DESTDIR)$(cherokeeconfdir)/mods-available/ssl ; then \
		echo "$@ will not overwrite existing $(DESTDIR)$(cherokeeconfdir)/mods-available/ssl"; \
	else \
		$(INSTALL_DATA) $(top_builddir)/mods-ssl.sample $(DESTDIR)$(cherokeeconfdir)/mods-available/ssl; \
	fi
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)

uninstall-local: uninstall-local-config
	@if test -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf ; then \
		echo "rm -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf"; \
		$(RM) -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf; \
	fi
	@if test -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf.perf_sample ; then \
		echo "rm -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf.perf_sample"; \
		$(RM) -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf.perf_sample; \
	fi
	@if test -f $(DESTDIR)$(cherokeeconfdir)/mime.types ; then \
		echo "rm -f $(cherokeeconfdir)/mime.types"; \
		$(RM) -f $(DESTDIR)$(cherokeeconfdir)/mime.types; \
	fi
	@if test -f $(DESTDIR)$(cherokeeconfdir)/mime.compression.types ; then \
		echo "rm -f $(cherokeeconfdir)/mime.compression.types"; \
		$(RM) -f $(DESTDIR)$(cherokeeconfdir)/mime.compression.types; \
	fi
	@if test -f $(DESTDIR)$(cherokeeconfdir)/mods-available/ssl ; then \
		echo "rm -f $(cherokeeconfdir)/mods-available/ssl"; \
		$(RM) -f $(DESTDIR)$(cherokeeconfdir)/mods-available/ssl; \
	fi
	@if test -f $(DESTDIR)$(cherokeeconfdir)/mods-available/icons ; then \
		echo "rm -f $(cherokeeconfdir)/mods-available/icons"; \
		$(RM) -f $(DESTDIR)$(cherokeeconfdir)/mods-available/icons; \
	fi

test:
	$(MAKE) -C qa test

