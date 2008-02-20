## Cherokee: Makefile.am  -*- makefile -*-

SUBDIRS = static

SUFFIXES = .py.pre .py

.py.pre.py:
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%datadir%|${datadir}|g; s|%prefix%|${prefix}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%version%|${PACKAGE_VERSION}|g" $< > $@

PY_PRE = \
configured.py.pre

generateddir = "$(datadir)/cherokee/admin"
generated_DATA = \
$(PY_PRE:.py.pre=.py)

adminpydir = "$(datadir)/cherokee/admin"
adminpy_DATA = \
consts.py \
Entry.py \
Form.py \
Page.py \
PageAjaxUpdate.py \
PageAdvanced.py \
PageEntry.py \
PageGeneral.py \
PageIcon.py \
PageMain.py \
PageMime.py \
PageVServer.py \
PageVServers.py \
PageEncoders.py \
PageError.py \
Table.py \
Theme.py \
VirtualServer.py \
Module.py \
ModuleFile.py \
ModuleDirlist.py \
ModuleCommon.py \
ModuleRedir.py \
ModuleServerInfo.py \
ModuleCgi.py \
ModuleFcgi.py \
ModuleScgi.py \
ModuleAuth.py \
ModuleAdmin.py \
ModuleHtdigest.py \
ModuleLdap.py \
ModulePlain.py \
ModuleHtpasswd.py \
ModuleMysql.py \
config.py \
pyscgi.py \
server.py \
validations.py \
CherokeeManagement.py

adminhelpdir = "$(datadir)/cherokee/admin"
adminhelp_DATA = \
default.template.html \
error_not_writable.template.html \
error_not_found.template.html \
error_icons_dir_missing.template.html \
advanced.help.html \
entry.help.html \
general.help.html \
icon.help.html \
main.help.html \
main.template.html \
mime.help.html \
vserver.help.html \
vservers.help.html \
encoder.help.html


EXTRA_DIST = \
$(PY_PRE) \
$(adminpy_DATA) \
$(adminhelp_DATA)

CLEANFILES = \
$(generated_DATA)
