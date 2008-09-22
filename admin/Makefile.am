## Cherokee: Makefile.am  -*- makefile -*-

if INSTALL_ADMIN
SUBDIRS = static
endif

SUFFIXES = .py.pre .py

.py.pre.py:
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%datadir%|${datadir}|g; s|%docdir%|${docdir}|g; s|%prefix%|${prefix}|g; s|%localstatedir%|${localstatedir}|g; s|%libdir%|${libdir}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%version%|${PACKAGE_VERSION}|g; s|%phpcgi%|${PHPCGI}|g" $< > $@

PY_PRE = \
configured.py.pre


if INSTALL_ADMIN
gen_data = $(PY_PRE:.py.pre=.py)
endif

generateddir = "$(datadir)/cherokee/admin"
generated_DATA = $(gen_data)


if INSTALL_ADMIN
admin_scripts = server.py
endif

adminpydir = "$(datadir)/cherokee/admin"
adminpy_SCRIPTS = $(admin_scripts)


if INSTALL_ADMIN
admin_data = \
consts.py \
Entry.py \
Form.py \
Post.py \
Page.py \
PageAjaxUpdate.py \
PageAdvanced.py \
PageEntry.py \
PageGeneral.py \
PageIcon.py \
PageStatus.py \
PageMime.py \
PageVServer.py \
PageVServers.py \
PageError.py \
PageFeedback.py \
PageInfoSource.py \
Table.py \
Theme.py \
RuleList.py \
Module.py \
ModuleHandler.py \
ModuleFile.py \
ModuleDirlist.py \
ModuleCommon.py \
ModuleRedir.py \
ModuleServerInfo.py \
ModuleCgi.py \
ModuleFcgi.py \
ModuleScgi.py \
ModuleBalancer.py \
ModuleMirror.py \
ModuleDbslayer.py \
ModuleErrorNn.py \
ModuleErrorRedir.py \
ModuleAuth.py \
ModuleAdmin.py \
ModuleHtdigest.py \
ModuleLdap.py \
ModulePlain.py \
ModuleHtpasswd.py \
ModuleMysql.py \
ModulePam.py \
ModuleRoundRobin.py \
ModuleDefault.py \
ModuleRequest.py \
ModuleDirectory.py \
ModuleExtensions.py \
ModuleGeoip.py \
ModuleHeader.py \
ModuleCustomError.py \
config.py \
pyscgi.py \
validations.py \
CherokeeManagement.py \
flags.py
endif

adminpy_DATA = $(admin_data)


if INSTALL_ADMIN
template_data = \
default.template.html \
status.template.html \
error_not_writable.template.html \
error_not_found.template.html \
error_icons_dir_missing.template.html \
error_couldnt_launch.template.html
endif

adminhelpdir = "$(datadir)/cherokee/admin"
adminhelp_DATA = $(template_data)

EXTRA_DIST = \
$(PY_PRE) \
$(admin_data) \
$(admin_scripts) \
$(template_data)

CLEANFILES = \
$(generated_DATA)
