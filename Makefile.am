## Cherokee: Makefile.am  -*- makefile -*-

SUBDIRS = m4 www icons themes qa doc . admin contrib cherokee cget windows
bin_SCRIPTS = cherokee-config
SUFFIXES = .sample.pre .sample .h.pre .h

# M4 macro file for inclusion with autoconf
m4datadir = "$(datadir)/aclocal"
m4data_DATA = cherokee.m4

# Man pag
man_MANS =           \
cherokee.1           \
cherokee-config.1    \
spawn-fcgi.1         \
cherokee-tweak.1     \
cherokee-admin.1     \
cherokee-worker.1

# pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = cherokee.pc

# Configuration files
cherokeeconfdir = $(sysconfdir)/cherokee
cherokeeadmindir = $(datadir)/cherokee/admin

.h.pre.h: 
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%datadir%|${datadir}|g; s|%docdir%|${docdir}|g; s|%prefix%|${prefix}|g; s|%libdir%|${libdir}|g; s|%localstatedir%|${localstatedir}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%phpcgi%|${PHPCGI}|g" $< > $@
.sample.pre.sample:
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%datadir%|${datadir}|g; s|%docdir%|${docdir}|g; s|%prefix%|${prefix}|g; s|%libdir%|${libdir}|g; s|%localstatedir%|${localstatedir}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%phpcgi%|${PHPCGI}|g" $< > $@


CONSTANTS_PRE = \
constants.h.pre

CONSTANTS = \
$(CONSTANTS_PRE:.h.pre=.h)


CONFS_PRE =              \
cherokee.conf.sample.pre \
performance.conf.sample.pre

CONFS = \
$(CONFS_PRE:.sample.pre=.sample)

WINDOWS_PORT_FILES = \
config.h.win32

CLEANFILES = \
$(CONFS) \
$(CONSTANTS)

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
$(CONFS_PRE)          \
$(CONSTANTS_PRE)      \
$(man_MANS)           \
$(WINDOWS_PORT_FILES) \
$(COMPILATION_TESTS)

all-local: $(CONSTANTS) $(CONFS)

install-data-local-config:
	@$(mkinstalldirs) $(DESTDIR)$(pkgincludedir);
	$(INSTALL_DATA) $(top_builddir)/config.h $(DESTDIR)$(pkgincludedir)/cherokee-config.h

uninstall-local-config:
	@if test -f $(DESTDIR)$(pkgincludedir)/cherokee-config.h; then \
		echo "rm -f $(DESTDIR)$(pkgincludedir)/cherokee-config.h"; \
		$(RM) -f $(DESTDIR)$(pkgincludedir)/cherokee-config.h; \
	fi

install-data-local: $(CONFS) install-data-local-config
	@$(mkinstalldirs) $(DESTDIR)$(localstatedir)
	@$(mkinstalldirs) $(DESTDIR)$(localstatedir)/run
	@$(mkinstalldirs) $(DESTDIR)$(localstatedir)/log
	@$(mkinstalldirs) $(DESTDIR)$(cherokeeconfdir)
	@$(mkinstalldirs) $(DESTDIR)$(cherokeeconfdir)/ssl
	@$(mkinstalldirs) $(DESTDIR)$(cherokeeadmindir)
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

test:
	$(MAKE) -C qa test
