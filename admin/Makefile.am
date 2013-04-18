## Cherokee: Makefile.am  -*- makefile -*-

SUBDIRS = plugins wizards CTK icons

if INSTALL_ADMIN
SUBDIRS += static
endif

SUFFIXES = .py.pre .py

.py.pre.py:
	sed -e "s|%sysconfdir%|${sysconfdir}|g; s|%sbindir%|${sbindir}|g; s|%docdir%|${docdir}|g; s|%prefix%|${prefix}|g; s|%localstatedir%|${localstatedir}|g; s|%libdir%|${libdir}|g; s|%wwwroot%|${WWW_ROOT}|g; s|%version%|${PACKAGE_VERSION}|g; s|%phpcgi%|${PHPCGI}|g; s|%datadir%|${datadir}|g; s|%localedir%|${localedir}|g" $< > $@

PY_PRE = \
configured.py.pre


if INSTALL_ADMIN
gen_data = $(PY_PRE:.py.pre=.py)
endif

generateddir = "$(datadir)/cherokee/admin"
generated_DATA = $(gen_data)


if INSTALL_ADMIN
admin_scripts = server.py upgrade_config.py
endif

adminpydir = "$(datadir)/cherokee/admin"
adminpy_SCRIPTS = $(admin_scripts)


if INSTALL_ADMIN
admin_data = \
About.py \
Auth.py \
Balancer.py \
Certs.py \
CgiBase.py \
Cherokee.py \
Flags.py \
Graph.py \
Handler.py \
Icons.py \
Mime.py \
Page.py \
PageAdvanced.py \
PageEntry.py \
PageError.py \
PageException.py \
PageGeneral.py \
PageHelp.py \
PageIndex.py \
PageNewConfig.py \
PageRule.py \
PageSource.py \
PageSources.py \
PageStatus.py \
PageVServer.py \
PageVServers.py \
Rule.py \
RuleSimple.py \
SaveButton.py \
SelectionPanel.py \
SystemInfo.py \
SystemStats.py \
SystemStatsWidgets.py \
Wizard.py \
XMLServerDigest.py \
config_version.py \
configured.py \
consts.py \
util.py \
popen.py \
validations.py \
SavingChecks.py

html_data = \
exception.html \
help.html \
theme.html

endif

adminpy_DATA = $(admin_data) $(html_data)

if INSTALL_ADMIN
template_data =
endif

adminhelpdir = "$(datadir)/cherokee/admin"
adminhelp_DATA = $(template_data)

EXTRA_DIST = \
$(PY_PRE) \
$(html_data) \
$(admin_data) \
$(admin_scripts) \
$(template_data)

CLEANFILES = \
$(generated_DATA)

test:
	python -m compileall .
