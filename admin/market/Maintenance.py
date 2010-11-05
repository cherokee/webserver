# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2010 Alvaro Lopez Ortega
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#

import os
import re
import CTK
import Page
import stat
import time
import popen
import OWS_Login

from util import *
from consts import *
from ows_consts import *
from configured import *

URL_MAINTENANCE_LIST       = "/market/maintenance/list"
URL_MAINTENANCE_LIST_APPLY = "/market/maintenance/list/apply"


#
# Cache
#

orphan_installs_cache     = None
unfinished_installs_cache = None

def Invalidate_Cache():
    global orphan_installs_cache
    global unfinished_installs_cache

    orphan_installs_cache     = None
    unfinished_installs_cache = None


#
# Logic
#

def check_unfinished_installations():
    # Check the cache
    global unfinished_installs_cache
    if unfinished_installs_cache:
        return unfinished_installs_cache

    # Check all the applications
    unfinished = []

    if not os.path.exists (CHEROKEE_OWS_ROOT) or \
       not os.access (CHEROKEE_OWS_ROOT, os.R_OK):
        return unfinished

    for d in os.listdir (CHEROKEE_OWS_ROOT):
        fd = os.path.join (CHEROKEE_OWS_ROOT, d)

        if not all([c in "0123456789." for c in d]):
            continue

        if not os.path.isdir(fd):
            continue

        finished = os.path.join (fd, 'finished')
        if not os.path.exists (finished):
            unfinished.append (d)

    unfinished_installs_cache = unfinished
    return unfinished


def check_orphan_installations():
    # Check the cache
    global orphan_installs_cache
    if orphan_installs_cache:
        return orphan_installs_cache

    # Check the config file
    orphans = []
    cfg_txt = CTK.cfg.serialize()

    if not os.path.exists (CHEROKEE_OWS_ROOT) or \
       not os.access (CHEROKEE_OWS_ROOT, os.R_OK):
        return unfinished

    for d in os.listdir (CHEROKEE_OWS_ROOT):
        fd = os.path.join (CHEROKEE_OWS_ROOT, d)

        if not fd in cfg_txt:
            orphans.append(d)

    orphan_installs_cache = orphans
    return orphans


def does_it_need_maintenance():
    if len(check_orphan_installations()) > 0:
        return True

    if len(check_unfinished_installations()) > 0:
        return True

    return False


def remove_app_databse (app):
    None


#
# GUI
#

class MaintenanceDialog (CTK.Dialog):
    def __init__ (self):
        CTK.Dialog.__init__ (self, {'title': _("Maintenance"), 'width': 600, 'minHeight': 300})

        self.refresh = CTK.RefreshableURL()
        self.druid = CTK.Druid(self.refresh)
        self.druid.bind ('druid_exiting', self.JS_to_close())
        self += self.druid

    def JS_to_show (self):
        js = CTK.Dialog.JS_to_show (self)
        js += self.refresh.JS_to_load (URL_MAINTENANCE_LIST)
        return js


class AppList (CTK.Table):
    def __init__ (self, apps):
        CTK.Table.__init__ (self, {'id': 'maintenance-removal-list'})

        self += [CTK.RawHTML(x) for x in (_('Remove'), _('Application'), _('Status'), _('Date'))]
        self.set_header()

        for app in apps:
            check = CTK.Checkbox ({'name': 'remove_%s'%(app), 'class': 'noauto'})
            self += [check,
                     CTK.RawHTML (apps[app]['name']),
                     CTK.RawHTML (apps[app]['type']),
                     CTK.RawHTML (apps[app]['date'])]


class ListApps:
    def __call__ (self):
        # Build list of apps to remove
        remove_apps = {}

        for app in check_orphan_installations():
            remove_apps[app] = {}
            remove_apps[app]['type'] = _('Orphan application')
            remove_apps[app]['name'] = self._figure_app_name (app)
            remove_apps[app]['date'] = self._figure_app_date (app)

        for app in check_unfinished_installations():
            if not app in remove_apps:
                remove_apps[app] = {}
                remove_apps[app]['type'] = _('Unfinished installation')
                remove_apps[app]['name'] = self._figure_app_name (app)
                remove_apps[app]['date'] = self._figure_app_date (app)

        # Dialog buttons
        buttons  = CTK.DruidButtonsPanel()
        buttons += CTK.DruidButton_Close (_('Cancel'))
        buttons += CTK.DruidButton_Submit (_('Remove'))

        # Content
        cont  = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_("Applications requiring maintenance")))
        cont += AppList (remove_apps)
        cont += buttons

        submit = CTK.Submitter (URL_MAINTENANCE_LIST_APPLY)
        submit += cont
        return submit.Render().toStr()

    def _figure_app_date (self, app):
        app_date = _('Unknown')
        try:
            fp = os.path.join (CHEROKEE_OWS_ROOT, app)
            app_date = time.ctime (os.stat(fp)[stat.ST_CTIME])
        except:
            pass
        return app_date

    def _figure_app_name (self, app):
        app_name = _('Unknown')
        try:
            fp = os.path.join (CHEROKEE_OWS_ROOT, app, 'install.log')
            if os.access (fp, os.R_OK):
                cont = open(fp, 'r').read()
                tmp = re.findall (r'Checking\: (.+)\, ID', cont, re.M)
                if tmp:
                    app_name = tmp[0]
        except:
            pass
        return app_name


def ListApps_Apply():
    # Check the apps to remove
    apps_to_remove = []

    for k in CTK.post.keys():
        if k.startswith ('remove_'):
            app = k[7:]
            if CTK.post.get_val(k) == '1':
                apps_to_remove.append (app)

    # Perform the app removal
    for app in apps_to_remove:
        # App Document root
        fp = os.path.join (CHEROKEE_OWS_ROOT, app)
        popen.popen_sync ("rm -rf '%s'" %(fp))

        # Database removal
        remove_app_databse (app)

    # The cache is no longer valid
    Invalidate_Cache()

    return CTK.cfg_reply_ajax_ok()


class Maintenance_Box (CTK.Box):
    def __init__ (self, refresh):
        CTK.Box.__init__ (self)

        unfinished = check_unfinished_installations()
        orphan     = check_orphan_installations()

        if not len(unfinished) and not len(orphan):
            return

        dialog = MaintenanceDialog()
        self += CTK.RawHTML ('<h3>%s</h3>' %(_('Maintanance')))
        self += dialog

        if len(unfinished):
            link = CTK.Link (None, CTK.RawHTML(_('Clean up')))
            link.bind ('click', dialog.JS_to_show())

            box = CTK.Box()
            box += CTK.RawHTML ('%d %s' %(len(unfinished), _("partial installations: ")))
            box += link

            self += box

        if len(orphan):
            link = CTK.Link (None, CTK.RawHTML(_('Clean up')))
            link.bind ('click', dialog.JS_to_show())

            box = CTK.Box()
            box += CTK.RawHTML ('%d %s' %(len(orphan), _("orphan installations: ")))
            box += link

            self += box


CTK.publish ('^%s$'%(URL_MAINTENANCE_LIST),       ListApps)
CTK.publish ('^%s$'%(URL_MAINTENANCE_LIST_APPLY), ListApps_Apply, method="POST")
