## Cherokee: Makefile.am  -*- makefile -*-

SUBDIRS = 

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
PageNewConfig.py \
PageVServer.py \
PageVServers.py \
PageError.py \
PageFeedback.py \
PageInfoSource.py \
Table.py \
Theme.py \
Rule.py \
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
ModuleUwsgi.py \
ModuleBalancer.py \
ModuleSsi.py \
ModuleSecdownload.py \
ModuleEmptyGif.py \
ModuleProxy.py \
ModuleDbslayer.py \
ModuleStreaming.py \
ModuleErrorNn.py \
ModuleErrorRedir.py \
ModuleAuth.py \
ModuleAuthlist.py \
ModuleAdmin.py \
ModuleHtdigest.py \
ModuleLdap.py \
ModulePlain.py \
ModuleHtpasswd.py \
ModuleMysql.py \
ModulePam.py \
ModuleRoundRobin.py \
ModuleIpHash.py \
ModuleDefault.py \
ModuleRequest.py \
ModuleDirectory.py \
ModuleExtensions.py \
ModuleFullpath.py \
ModuleGeoip.py \
ModuleHeader.py \
ModuleExists.py \
ModuleMethod.py \
ModuleBind.py \
ModuleFrom.py \
ModuleCustomError.py \
ModuleWildcard.py \
ModuleRehost.py \
ModuleTargetIp.py \
ModuleEvhost.py \
ModuleRrd.py \
ModuleUrlArg.py \
config.py \
config_version.py \
pyscgi.py \
validations.py \
CherokeeManagement.py \
GraphManager.py \
flags.py \
util.py \
Wizard.py \
Wizard_PHP.py \
Wizard_WordPress.py \
Wizard_Django.py \
Wizard_Redirect.py \
Wizard_HotLinking.py \
Wizard_Drupal.py \
Wizard_RoR.py \
Wizard_Mailman.py \
Wizard_Icons.py \
Wizard_CommonStatic.py \
Wizard_Streaming.py \
Wizard_rTorrent.py \
Wizard_Joomla.py \
Wizard_Moodle.py \
Wizard_Sugar.py \
Wizard_phpBB.py \
Wizard_phpMyAdmin.py \
Wizard_Symfony.py \
Wizard_Zend.py \
Wizard_Trac.py \
Wizard_Liferay.py \
Wizard_Alfresco.py \
Wizard_Glassfish.py \
Wizard_uWSGI.py \
Wizard_Mono.py \
Wizard_ColdFusion.py \
Wizard_Concrete5.py
endif

adminpy_DATA = $(admin_data)


if INSTALL_ADMIN
template_data = \
default.template.html \
status.template.html \
error_not_writable.template.html \
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
