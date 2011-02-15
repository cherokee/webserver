# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2011 Alvaro Lopez Ortega
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
import string
import OWS_Login
import SystemInfo
import CommandProgress

from util import *
from consts import *
from ows_consts import *
from configured import *

NOTE_DBUSER = N_("Specify the name of a privileged DB user. It will be used to remove the databases.")
NOTE_DBPASS = N_("Specify the password for this user account.")
DB_DEL_P1   = N_("Please, provide an administrative user/password pair to connect to your database.")

URL_MAINTENANCE_LIST       = "/market/maintenance/list"
URL_MAINTENANCE_LIST_APPLY = "/market/maintenance/list/apply"
URL_MAINTENANCE_REMOVE     = "/market/maintenance/remove"
URL_MAINTENANCE_DB         = "/market/maintenance/db"
URL_MAINTENANCE_DB_APPLY   = "/market/maintenance/db/apply"
URL_MAINTENANCE_FINISHED   = "/market/maintenance/finished"

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

        if not all([c in string.digits for c in d]):
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
        return orphans

    for d in os.listdir (CHEROKEE_OWS_ROOT):
        fd = os.path.join (CHEROKEE_OWS_ROOT, d)

        if not re.findall (r'document_root.*'+fd, cfg_txt, re.M):
            # Check usage in information sources
            if not used_in_active_sources (d):
                orphans.append(d)

    orphan_installs_cache = orphans
    return orphans


def used_in_sources (installation_id):
    directory = os.path.join (CHEROKEE_OWS_ROOT, installation_id)

    sources = []
    for src in CTK.cfg.keys("source"):
        interpreter = CTK.cfg.get_val("source!%s!interpreter" %(src))
        if interpreter and (directory in interpreter):
            sources.append (src)

    return sources


def used_in_active_sources (installation_id):
    sources = used_in_sources (installation_id)

    src_list = []
    for src_num in sources:
        for vsrv in CTK.cfg.keys("vserver"):
            for rule in CTK.cfg.keys("vserver!%s" %(vsrv)):
                for s in CTK.cfg.keys("vserver!%s!rule!%s!handler!balancer!source" %(vsrv, rule)):
                    value = CTK.cfg.get_val ("vserver!%s!rule!%s!handler!balancer!source!%s" %(vsrv, rule, s))
                    if value == src_num:
                        src_list (src_num)

    return src_list


def does_it_need_maintenance():
    if len(check_orphan_installations()) > 0:
        return True

    if len(check_unfinished_installations()) > 0:
        return True

    return False


def app_database_exists (app):
    fp = os.path.join (CHEROKEE_OWS_ROOT, app, "install.log")
    if os.access (fp, os.R_OK):
        cont = open(fp, 'r').read()

        # MySQL
        if 'DB: Created MySQL database' in cont:
            return "MySQL"
        if 'DB: Created PostgreSQL database' in cont:
            return "PostgreSQL"


def app_database_remove (app, user, passw, db_type):
    # MySQL
    if db_type == 'MySQL':
        params = "-u'%(user)s'" %(locals())
        if passw:
            params += " -p'%(passw)s'" %(locals())
        cmd = "mysql %(params)s -e 'DROP DATABASE IF EXISTS market_%(app)s'" %(locals())

    # PostgreSQL
    elif db_type == 'PostgreSQL':
        cmd = "PGUSER='%(user)s' PGPASSWORD='%(passw)s' psql -c 'DROP DATABASE IF EXISTS market_%(app)s'" %(locals())

    else:
        return

    ret = popen.popen_sync (cmd)
    if ret['retcode'] != 0:
        return ret['stderr']


#
# GUI
#

class Maintenance_Box (CTK.Box):
    def __init__ (self, refresh):
        CTK.Box.__init__ (self)

        # Check apps
        unfinished = check_unfinished_installations()
        orphan     = check_orphan_installations()

        if not len(unfinished) and not len(orphan):
            return

        # Unfinished apps are not even orphans
        for u in unfinished:
            if u in orphan:
                orphan.remove(u)

        # GUI
        dialog = MaintenanceDialog()
        dialog.bind ('dialogclose', refresh.JS_to_refresh())

        self += CTK.RawHTML ('<h3>%s</h3>' %(_('Maintanance')))
        self += dialog

        link = CTK.Link (None, CTK.RawHTML(_('Clean up')))
        link.bind ('click', dialog.JS_to_show())

        box = CTK.Box({'id':'market_maintenance_box'})
        if len(unfinished) and len(orphan):
            box += CTK.RawHTML (_("Detected %(num_orphan)d orphan, and %(num_unfinished)d unfinished installations: ") %(
                    {'num_orphan': len(orphan), 'num_unfinished': len(unfinished)}))
        elif len(unfinished):
            box += CTK.RawHTML (_("Detected %(num_unfinished)d unfinished installations: ") %(
                    {'num_unfinished': len(unfinished)}))
        elif len(orphan):
            box += CTK.RawHTML (_("Detected %(num_orphan)d orphan installations: ") %(
                    {'num_orphan': len(orphan)}))
        box += link
        self += box


class MaintenanceDialog (CTK.Dialog):
    def __init__ (self):
        CTK.Dialog.__init__ (self, {'title': _("Maintenance"), 'width': 600, 'minHeight': 300})

        self.refresh = CTK.RefreshableURL()
        self.druid = CTK.Druid (self.refresh)
        self.druid.bind ('druid_exiting', self.JS_to_close())
        self += self.druid

    def JS_to_show (self):
        js = CTK.Dialog.JS_to_show (self)
        js += self.refresh.JS_to_load (URL_MAINTENANCE_LIST)
        return js


class AppList (CTK.Table):
    def __init__ (self, apps, b_next, b_cancel, b_close):
        CTK.Table.__init__ (self, {'id': 'maintenance-removal-list'})

        # Dialog button management
        js = "if ($('#%s input:checked').size() > 0) {" %(self.id)
        js +=  b_close.JS_to_hide()
        js +=  b_cancel.JS_to_show()
        js +=  b_next.JS_to_show()
        js += "} else {"
        js +=  b_close.JS_to_show()
        js +=  b_cancel.JS_to_hide()
        js +=  b_next.JS_to_hide()
        js += "}"

        # Global Selector
        global_selector = CTK.Checkbox ({'class': 'noauto'})
        global_selector.bind ('change', """
            var is_checked = this.checked;
            $('#%s input:checkbox').each (function() {
                $(this).attr('checked', is_checked);
            });
        """ %(self.id) + js)

        # Add the table title
        title = [global_selector]
        title += [CTK.RawHTML(x) for x in (_('Application'), _('Status'), _('Database'), _('Date'))]

        self += title
        self.set_header()

        # Table body
        for app in apps:
            check = CTK.Checkbox ({'name': 'remove_%s'%(app), 'class': 'noauto'})
            check.bind ('change', js)
            self += [check,
                     CTK.RawHTML (apps[app]['name']),
                     CTK.RawHTML (apps[app]['type']),
                     CTK.RawHTML (apps[app].get('db', '')),
                     CTK.RawHTML (apps[app]['date'])]


class ListApps:
    def __call__ (self):
        # Check the apps
        orphan     = check_orphan_installations()
        unfinished = check_unfinished_installations()

        # Unfinished apps are not even orphans
        for u in unfinished:
            if u in orphan:
                orphan.remove(u)

        # Build list of apps to remove
        remove_apps = {}

        for app in orphan:
            db      = app_database_exists (app)
            service = self._figure_app_service (app)

            remove_apps[app] = {}
            remove_apps[app]['type'] = _('Orphan')
            remove_apps[app]['name'] = self._figure_app_name (app)
            remove_apps[app]['date'] = self._figure_app_date (app)
            if db:
                remove_apps[app]['db'] = db
            if service:
                remove_apps[app]['service'] = service

        for app in unfinished:
            if app in remove_apps:
                continue

            db      = app_database_exists (app)
            service = self._figure_app_service (app)
            app_fp  = os.path.join (CHEROKEE_OWS_ROOT, app)

            remove_apps[app] = {}
            remove_apps[app]['type'] = _('Unfinished')
            remove_apps[app]['name'] = self._figure_app_name (app)
            remove_apps[app]['date'] = self._figure_app_date (app)
            if db:
                remove_apps[app]['db'] = db
            if service:
                remove_apps[app]['service'] = service

            # Even though it's a 'unfinished' app, a virtual server
            # might exist already.
            for v in CTK.cfg['vserver'] or []:
                if CTK.cfg.get_val('vserver!%s!document_root'%(v)) == app_fp:
                    remove_apps[app]['vserver'] = v

        # Store in CTK.cfg
        del (CTK.cfg ['tmp!market!maintenance!remove'])
        for app in remove_apps:
            CTK.cfg ['tmp!market!maintenance!remove!%s!del'     %(app)] = 0
            CTK.cfg ['tmp!market!maintenance!remove!%s!name'    %(app)] = remove_apps[app]['name']
            CTK.cfg ['tmp!market!maintenance!remove!%s!db'      %(app)] = remove_apps[app].get('db')
            CTK.cfg ['tmp!market!maintenance!remove!%s!service' %(app)] = remove_apps[app].get('service')
            CTK.cfg ['tmp!market!maintenance!remove!%s!date'    %(app)] = remove_apps[app].get('date')
            CTK.cfg ['tmp!market!maintenance!remove!%s!vserver' %(app)] = remove_apps[app].get('vserver')

        # Dialog buttons
        b_next   = CTK.DruidButton_Goto  (_('Remove'), URL_MAINTENANCE_DB, True)
        b_close  = CTK.DruidButton_Close (_('Close'))
        b_cancel = CTK.DruidButton_Close (_('Cancel'))

        buttons  = CTK.DruidButtonsPanel()
        buttons += b_close
        buttons += b_cancel
        buttons += b_next

        # App list
        app_list = AppList (remove_apps, b_next, b_cancel, b_close)

        # Content
        cont  = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_("Applications requiring maintenance")))
        cont += CTK.Box ({'id': 'maintenance-removal'}, app_list)
        cont += buttons
        cont += CTK.RawHTML (js = b_next.JS_to_hide())
        cont += CTK.RawHTML (js = b_cancel.JS_to_hide())

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

        fp = os.path.join (CHEROKEE_OWS_ROOT, app, 'install.log')
        if os.access (fp, os.R_OK):
            cont = open(fp, 'r').read()
            tmp = re.findall (r'Checking\: (.+)\, ID', cont, re.M)
            if tmp:
                app_name = tmp[0]

        return app_name

    def _figure_app_service (self, app):
        fp = os.path.join (CHEROKEE_OWS_ROOT, app, 'install.log')
        if os.access (fp, os.R_OK):
            cont = open(fp, 'r').read()

            tmp = re.findall (r'Registered system service\: (.+)\n', cont, re.M)
            if tmp:
                return tmp[0]

            tmp = re.findall (r'Registered Launchd service\: (.+)\n', cont, re.M)
            if tmp:
                return tmp[0]

            tmp = re.findall (r'Registered BSD service\: (.+)\n', cont, re.M)
            if tmp:
                return tmp[0]


def ListApps_Apply():
    system_info = SystemInfo.get_info()
    OS = system_info.get('system','').lower()

    # Check the apps to remove
    apps_to_remove = []

    for k in CTK.post.keys():
        if k.startswith ('remove_'):
            app = k[7:]
            if CTK.post.get_val(k) == '1':
                apps_to_remove.append (app)

    # Something to do?
    if not apps_to_remove:
        return {'ret': 'fail'}

    # Store apps to remove
    for app in apps_to_remove:
        CTK.cfg ['tmp!market!maintenance!remove!%s!del' %(app)] = '1'

    return CTK.cfg_reply_ajax_ok()


class DatabaseRemoval:
    def __call__ (self):
        # Check the application
        db_type  = None
        db_found = False

        for app in CTK.cfg.keys ('tmp!market!maintenance!remove'):
            tmp = CTK.cfg.get_val ('tmp!market!maintenance!remove!%s!db'%(app))
            if not tmp:
                continue

            if not CTK.cfg.get_val ('tmp!market!maintenance!db!%s!user'%(tmp)):
                db_found = True
                db_type  = tmp

        # No DBs, we are done here
        if not db_found:
            box = CTK.Box()
            box += CTK.RawHTML (js = CTK.DruidContent__JS_to_goto (box.id, URL_MAINTENANCE_REMOVE))
            return box.Render().toStr()

        # Ask for the user and password
        table = CTK.PropsTable()
        table.Add (_('DB user'),     CTK.TextField        ({'name':'db_user', 'class':'noauto', 'value':'root'}), _(NOTE_DBUSER))
        table.Add (_('DB password'), CTK.TextFieldPassword({'name':'db_pass', 'class':'noauto'}), _(NOTE_DBPASS))

        # Reload its content on submit success
        submit  = CTK.Submitter (URL_MAINTENANCE_DB_APPLY)
        submit.bind ('submit_success', CTK.DruidContent__JS_to_goto (table.id, URL_MAINTENANCE_DB))
        submit += CTK.Hidden ('db_type', db_type)
        submit += table

        cont  = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s (%s)</h2>' %(_("Database Credentials"), db_type))
        cont += CTK.RawHTML ('<p>%s</p>'   %(_(DB_DEL_P1)))
        cont += submit

        return cont.Render().toStr()


def DatabaseRemoval_Apply():
    db_user = CTK.post.get_val('db_user')
    db_pass = CTK.post.get_val('db_pass')
    db_type = CTK.post.get_val('db_type')

    # Example:
    #  tmp!market!maintenance!db!mysql!user = root
    #  tmp!market!maintenance!db!mysql!pass = root

    CTK.cfg['tmp!market!maintenance!db!%s!user'%(db_type)] = db_user
    CTK.cfg['tmp!market!maintenance!db!%s!pass'%(db_type)] = db_pass

    return CTK.cfg_reply_ajax_ok()


def _remove_app (app):
    # Remove services
    service = CTK.cfg.get_val ('tmp!market!maintenance!remove!%s!service'%(app))
    if service:
        if OS == 'darwin':
            popen.popen_sync ('launchctl unload %(service)s' %(locals()))

        elif OS == 'linux':
            popen.popen_sync ('rm -f /etc/rcS.d/S99%(service)s' %(locals()))

        elif OS == 'freebsd':
            content = open('/etc/rc.conf', 'r').read()
            content = content.replace ('%s="YES"\n'%(service), '')
            open('/etc/rc.conf', 'w').write (content)

        print "Remove service", service

    # Remove unused info sources created by the app
    sources = used_in_sources (app)
    for src in sources:
        del (CTK.cfg ['source!%s' %(src)])

    # Perform the app removal
    fp = os.path.join (CHEROKEE_OWS_ROOT, app)
    popen.popen_sync ("rm -rf '%s'" %(fp))

    # Virtual Server
    vserver_to_remove = CTK.cfg.get_val ('tmp!market!maintenance!remove!%s!vserver'%(app))
    if vserver_to_remove:
        del (CTK.cfg['vserver!%s'%(vserver_to_remove)])

    # Database removal
    db_type = CTK.cfg.get_val ('tmp!market!maintenance!remove!%s!db'%(app))
    if db_type:
        db_user = CTK.cfg.get_val('tmp!market!maintenance!db!%s!user'%(db_type))
        db_pass = CTK.cfg.get_val('tmp!market!maintenance!db!%s!pass'%(db_type), '')

        app_database_remove (app, db_user, db_pass, db_type)

    # CTK.cfg clean up
    del (CTK.cfg['tmp!market!maintenance!remove!%s'%(app)])

    return {'retcode': 0}


class RemoveApps:
    def __call__ (self):
        commands = []

        for app in CTK.cfg.keys ('tmp!market!maintenance!remove'):
            # Should it be deleted?
            delete = CTK.cfg.get_val ('tmp!market!maintenance!remove!%s!del'%(app), 0)
            if not int(delete):
                continue

            # Build the command list
            name = CTK.cfg.get_val ('tmp!market!maintenance!remove!%s!name'%(app))
            date = CTK.cfg.get_val ('tmp!market!maintenance!remove!%s!date'%(app))

            entry = {}
            entry['description'] = _("Removing %(name)s (%(date)s)" %(locals()))
            entry['function']    = lambda app=app: _remove_app(app)

            commands += [entry]

        cont  = CTK.Container()
        cont += CTK.RawHTML ("<h2>%s</h2>" %(_("Removing Applications")))
        cont += CommandProgress.CommandProgress (commands, URL_MAINTENANCE_FINISHED)

        buttons = CTK.DruidButtonsPanel()
        buttons += CTK.DruidButton_Close(_('Cancel'))
        cont += buttons

        return cont.Render().toStr()


class Finished:
    def __call__ (self):
        # Clean up
        del (CTK.cfg ['tmp!market!maintenance'])

        # The cache is no longer valid
        Invalidate_Cache()

        # Close dialog
        box = CTK.Box()
        box += CTK.RawHTML (js = CTK.DruidContent__JS_to_close (box.id))
        return box.Render().toStr()


CTK.publish ('^%s$'%(URL_MAINTENANCE_LIST),       ListApps)
CTK.publish ('^%s$'%(URL_MAINTENANCE_LIST_APPLY), ListApps_Apply, method="POST")
CTK.publish ('^%s$'%(URL_MAINTENANCE_DB),         DatabaseRemoval)
CTK.publish ('^%s$'%(URL_MAINTENANCE_DB_APPLY),   DatabaseRemoval_Apply, method="POST")
CTK.publish ('^%s$'%(URL_MAINTENANCE_REMOVE),     RemoveApps)
CTK.publish ('^%s$'%(URL_MAINTENANCE_FINISHED),   Finished)
