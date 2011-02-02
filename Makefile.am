## Cherokee: Makefile.am  -*- makefile -*-

SUBDIRS = static CTK

bin_SCRIPTS = CTK-run

CTK-run: CTK-run.pre
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%sbindir%|${sbindir}|g; s|%docdir%|${docdir}|g; s|%prefix%|${prefix}|g; s|%localstatedir%|${localstatedir}|g; s|%libdir%|${libdir}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%version%|${PACKAGE_VERSION}|g; s|%phpcgi%|${PHPCGI}|g; s|%datadir%|${datadir}|g; s|%localedir%|${localedir}|g" $(top_srcdir)/admin/CTK/CTK-run.pre > CTK-run

EXTRA_DIST = \
CTK-run.pre

CLEANFILES = \
CTK-run
