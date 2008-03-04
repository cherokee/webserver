## Cherokee: Makefile.am  -*- makefile -*-

SUBDIRS = m4 contrib www icons doc themes qa cherokee cget windows admin
bin_SCRIPTS = cherokee-config
SUFFIXES = .sample.pre .sample

# M4 macro file for inclusion with autoconf
m4datadir = "$(datadir)/aclocal"
m4data_DATA = cherokee.m4

# Man pag
man_MANS =        \
cget.1            \
cherokee.1        \
cherokee-config.1 \
cherokee_logrotate.1

# pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = cherokee.pc

# Configuration files
cherokeeconfdir = $(sysconfdir)/cherokee
cherokeeadmindir = $(datadir)/cherokee/admin

.sample.pre.sample:
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%datadir%|${datadir}|g; s|%prefix%|${prefix}|g; s|%localstatedir%|${localstatedir}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%phpcgi%|${PHPCGI}|g" $< > $@

CONFS_PRE =              \
cherokee.conf.sample.pre \
performance.conf.sample.pre

CONFS_DONE =           \
mime.types.sample      \
mime.compression.types.sample

CONFS = \
$(CONFS_PRE:.sample.pre=.sample)

WINDOWS_PORT_FILES = \
config.h.win32

CLEANFILES = \
$(CONFS)

DISTCLEANFILES = \
cherokee.pc

COMPILATION_TESTS = \
debian_testing.sh   \
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
	$(INSTALL_DATA) $(top_builddir)/cherokee.conf.sample $(DESTDIR)$(cherokeeadmindir)/cherokee.conf.sample
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

test:
	$(MAKE) -C qa test
