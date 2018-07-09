## Cherokee: Makefile.am  -*- makefile -*-

SUBDIRS = po m4 www icons themes qa doc . admin contrib cherokee cget packages dbslayer
SUFFIXES = .sample.pre .sample .h.pre .h .xml.pre .xml
ACLOCAL_AMFLAGS = -I m4

bin_SCRIPTS = cherokee-config

# M4 macro file for inclusion with autoconf
m4datadir = "$(datadir)/aclocal"
m4data_DATA = cherokee.m4

# Man pag
man_MANS =                \
cherokee.1                \
cherokee-config.1         \
cherokee-tweak.1          \
cherokee-admin.1          \
cherokee-worker.1         \
cherokee-admin-launcher.1

# pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = cherokee.pc

# Configuration files
cherokeeconfdir = $(sysconfdir)/cherokee
cherokeeadmindir = $(datadir)/cherokee/admin

.h.pre.h:
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%sbindir%|${sbindir}|g; s|%docdir%|${docdir}|g; s|%prefix%|${prefix}|g; s|%localstatedir%|${localstatedir}|g; s|%libdir%|${libdir}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%cgiroot%|${CGI_ROOT}|g; s|%version%|${PACKAGE_VERSION}|g; s|%phpcgi%|${PHPCGI}|g; s|%datadir%|${datadir}|g; s|%localedir%|${localedir}|g" $< > $@
.sample.pre.sample:
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%sbindir%|${sbindir}|g; s|%docdir%|${docdir}|g; s|%prefix%|${prefix}|g; s|%localstatedir%|${localstatedir}|g; s|%libdir%|${libdir}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%cgiroot%|${CGI_ROOT}|g; s|%version%|${PACKAGE_VERSION}|g; s|%phpcgi%|${PHPCGI}|g; s|%datadir%|${datadir}|g; s|%localedir%|${localedir}|g" $< > $@
.xml.pre.xml:
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%sbindir%|${sbindir}|g; s|%docdir%|${docdir}|g; s|%prefix%|${prefix}|g; s|%localstatedir%|${localstatedir}|g; s|%libdir%|${libdir}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%cgiroot%|${CGI_ROOT}|g; s|%version%|${PACKAGE_VERSION}|g; s|%phpcgi%|${PHPCGI}|g; s|%datadir%|${datadir}|g; s|%localedir%|${localedir}|g" $< > $@


CONSTANTS_PRE = \
constants.h.pre

CONSTANTS = \
$(CONSTANTS_PRE:.h.pre=.h)

CONFS_PRE =              \
cherokee.conf.sample.pre \
performance.conf.sample.pre

CONFS = \
$(CONFS_PRE:.sample.pre=.sample)

XMLS_PRE = \
http-cherokee.xml.pre

XMLS = \
$(XMLS_PRE:.xml.pre=.xml)

CLEANFILES = \
$(XMLS) \
$(CONFS) \
$(CONSTANTS)

DISTCLEANFILES = \
cherokee.pc

COMPILATION_TESTS = \
debian_testing.sh   \
debian_testing_chroot.sh


EXTRA_DIST =                    \
http-cherokee.xml               \
cherokee.spec                   \
cherokee.spec.in                \
org.cherokee.webserver.plist    \
org.cherokee.webserver.plist.in \
SUNWcherokee.spec               \
pam.d_cherokee                  \
cherokee.pc.in                  \
developers.py                   \
$(m4data_DATA)                  \
$(XMLS_PRE)                     \
$(CONFS_PRE)                    \
$(CONSTANTS_PRE)                \
$(man_MANS)                     \
$(COMPILATION_TESTS)

all-local: $(CONSTANTS) $(CONFS) $(XMLS) add_user_group

add_user_group: cherokee.conf.sample
	@if test "x$(WWW_USER)" != "x" ; then \
		grep -e '^server.user' cherokee.conf.sample >/dev/null 2>/dev/null || ( \
			echo "Adding default user '$(WWW_USER)' to cherokee.conf.sample.."; \
			echo 'server!user = '$(WWW_USER) >> cherokee.conf.sample; \
		); \
	fi
	@if test "x$(WWW_GROUP)" != "x" ; then \
		grep -e '^server.group' cherokee.conf.sample >/dev/null 2>/dev/null || ( \
			echo "Adding default group '$(WWW_GROUP)' to cherokee.conf.sample.."; \
			echo 'server!group = '$(WWW_GROUP) >> cherokee.conf.sample; \
		); \
	fi

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
	@$(mkinstalldirs) $(DESTDIR)$(localstatedir)/lib/cherokee/graphs/images
	@$(mkinstalldirs) $(DESTDIR)$(cherokeeconfdir)
	@$(mkinstalldirs) $(DESTDIR)$(cherokeeadmindir)
	@if test -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf ; then \
		echo "$@ will not overwrite existing $(DESTDIR)$(cherokeeconfdir)/cherokee.conf"; \
	else \
		$(INSTALL_DATA) $(top_builddir)/cherokee.conf.sample $(DESTDIR)$(cherokeeconfdir)/cherokee.conf; \
	fi
	$(INSTALL_DATA) $(top_builddir)/cherokee.conf.sample $(DESTDIR)$(cherokeeadmindir)/cherokee.conf.sample
	$(INSTALL_DATA) $(top_builddir)/performance.conf.sample $(DESTDIR)$(cherokeeadmindir)/performance.conf.sample
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
#	@if test -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf ; then \
#		echo "rm -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf"; \
#		$(RM) -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf; \
#	fi
	@if test -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf.perf_sample ; then \
		echo "rm -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf.perf_sample"; \
		$(RM) -f $(DESTDIR)$(cherokeeconfdir)/cherokee.conf.perf_sample; \
	fi
	$(RM) -f $(DESTDIR)$(cherokeeadmindir)/cherokee.conf.sample
	$(RM) -f $(DESTDIR)$(cherokeeadmindir)/performance.conf.sample

test:
	$(MAKE) -C doc test
	$(MAKE) -C admin test
	$(MAKE) -C qa test

update-po:
	$(MAKE) -C po/admin update-po
