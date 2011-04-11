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

import CTK

import re
import os
import imp
import stat
import sys
import time
import tarfile
import traceback
import OWS_Login
import Maintenance
import Library
import Install_Log
import SystemInfo
import SystemStats
import SaveButton
import Cherokee
import Distro
import popen
import CommandProgress
import InstallUtil

from util import *
from consts import *
from ows_consts import *
from configured import *

from XMLServerDigest import XmlRpcServer

PAYMENT_CHECK_TIMEOUT  = 5 * 1000 # 5 secs

NOTE_ALREADY_INSTALLED = N_('The application is already in your library, so there is no need the buy it again. Please, proceed to the installation.')

NOTE_ALL_READY_TO_BUY_1  = N_('You are about to purchase the application. Please proceed with the Check Out to provide the payment details.')
NOTE_ALL_READY_TO_BUY_2  = N_('The application will be downloaded and installed afterwards, and will remain in your library for future installations.')

NOTE_THANKS_P1    = N_("Cherokee is now ready to run the application. Please, remember to backup your configuration if you are going to perform customizations.")
NOTE_THANKS_P2    = N_("Thank you for buying at Cherokee's Market!")
NOTE_SAVE_RESTART = N_("Since there were previous changes your configuration has not been applied automatically. Please do it yourself by clicking the SAVE button on the top-right corner.")

NO_ROOT_H1 = N_("Privileges test failed")
NO_ROOT_P1 = N_("Since the installations may require administrator to execute system administration commands, it is required to run Cherokee-admin under a user with  administrator privileges.")
NO_ROOT_P2 = N_("Please, run cherokee-admin as root to solve the problem.")

MIN_VER_H1 = N_("The package cannot be installed")
MIN_VER_p1 = N_("This package required at least Cherokee %(version)s to be installed.")


URL_INSTALL_WELCOME        = "%s/install/welcome"        %(URL_MAIN)
URL_INSTALL_INIT_CHECK     = "%s/install/check"          %(URL_MAIN)
URL_INSTALL_PAY_CHECK      = "%s/install/pay"            %(URL_MAIN)
URL_INSTALL_DOWNLOAD       = "%s/install/download"       %(URL_MAIN)
URL_INSTALL_DOWNLOAD_ERROR = "%s/install/download_error" %(URL_MAIN)
URL_INSTALL_SETUP          = "%s/install/setup"          %(URL_MAIN)
URL_INSTALL_POST_UNPACK    = "%s/install/post_unpack"    %(URL_MAIN)
URL_INSTALL_EXCEPTION      = "%s/install/exception"      %(URL_MAIN)
URL_INSTALL_SETUP_EXTERNAL = "%s/install/setup/package"  %(URL_MAIN)
URL_INSTALL_DONE           = "%s/install/done"           %(URL_MAIN)
URL_INSTALL_DONE_CONTENT   = "%s/install/done/content"   %(URL_MAIN)
URL_INSTALL_DONE_APPLY     = "%s/install/done/apply"     %(URL_MAIN)


class InstallDialog (CTK.Dialog):
    def __init__ (self, app_name):
        index = Distro.Index()
        app   = index.get_package (app_name, 'software')

        title = "%s  —  %s" %(app['name'], _("Cherokee Market"))

        CTK.Dialog.__init__ (self, {'title': title, 'width': 600, 'minHeight': 300})
        self.info = app

        # Copy a couple of preliminary config entries
        CTK.cfg['tmp!market!install!app!application_id']   = app['id']
        CTK.cfg['tmp!market!install!app!application_name'] = app['name']

        self.refresh = CTK.RefreshableURL()
        self.druid = CTK.Druid(self.refresh)
        self.druid.bind ('druid_exiting', self.JS_to_close())

        self += self.druid

    def JS_to_show (self):
        js = CTK.Dialog.JS_to_show (self)
        js += self.refresh.JS_to_load (URL_INSTALL_WELCOME)
        return js


class Install_Stage:
    def __call__ (self):
        try:
            return self.__safe_call__()
        except Exception, e:
            # Log the exception
            exception_str = traceback.format_exc()
            print exception_str
            Install_Log.log ("EXCEPTION!\n" + exception_str)

            # Reset 'unfinished installations' cache
            Maintenance.Invalidate_Cache()

            # Present an alternative response
            cont = Exception_Handler (exception_str)
            return cont.Render().toStr()


class Welcome (Install_Stage):
    def __safe_call__ (self):
        # Ensure the current UID has enough priviledges
        if not InstallUtil.current_UID_is_admin() and 0:
            box = CTK.Box()
            box += CTK.RawHTML ('<h2>%s</h2>' %(_(NO_ROOT_H1)))
            box += CTK.RawHTML ('<p>%s</p>'   %(_(NO_ROOT_P1)))
            box += CTK.RawHTML ('<p>%s</p>'   %(_(NO_ROOT_P2)))

            buttons = CTK.DruidButtonsPanel()
            buttons += CTK.DruidButton_Close(_('Cancel'))
            box += buttons

            return box.Render().toStr()

        # Check the rest of pre-requisites
        index  = Distro.Index()
        app_id = CTK.cfg.get_val('tmp!market!install!app!application_id')
        app    = index.get_package (app_id, 'software')

        inst = index.get_package (app_id, 'installation')
        if 'cherokee_min_ver' in inst:
            version = inst['cherokee_min_ver']
            if version_cmp (VERSION, version) < 0:
                box = CTK.Box()
                box += CTK.RawHTML ('<h2>%s</h2>' %(_(MIN_VER_H1)))
                box += CTK.RawHTML ('<p>%s</p>'   %(_(MIN_VER_P1)%(locals())))

                buttons = CTK.DruidButtonsPanel()
                buttons += CTK.DruidButton_Close(_('Cancel'))

                box += buttons
                return box.Render().toStr()

        # Init the log file
        Install_Log.reset()
        Install_Log.log (".---------------------------------------------.")
        Install_Log.log ("| PLEASE, DO NOT EDIT OR REMOVE THIS LOG FILE |")
        Install_Log.log ("|                                             |")
        Install_Log.log ("| It contains useful information that         |")
        Install_Log.log ("| cherokee-admin might need in the future.    |")
        Install_Log.log (".---------------------------------------------.")
        Install_Log.log ("Retrieving package information...")

        # Check whether there are CTK.cfg changes to be saved
        changes = "01"[int(CTK.cfg.has_changed())]
        CTK.cfg['tmp!market!install!cfg_previous_changes'] = changes

        # Render a welcome message
        box = CTK.Box()
        box += CTK.RawHTML (js = CTK.DruidContent__JS_to_goto (box.id, URL_INSTALL_DOWNLOAD))

        return box.Render().toStr()


class Download (Install_Stage):
    def __safe_call__ (self):
        app_id   = CTK.cfg.get_val('tmp!market!install!app!application_id')
        app_name = CTK.cfg.get_val('tmp!market!install!app!application_name')

        # URL Package
        index = Distro.Index()
        pkg   = index.get_package (app_id, 'package')

        repo_url = CTK.cfg.get_val ('admin!ows!repository', REPO_MAIN)
        url_download = os.path.join (repo_url, app_id, pkg['filename'])
        CTK.cfg['tmp!market!install!download'] = url_download

        # Local storage shortcut
        pkg_filename_full = url_download.split('/')[-1]
        pkg_filename = pkg_filename_full.split('_')[0]
        pkg_revision = 0

        pkg_repo_fp  = os.path.join (CHEROKEE_OWS_DIR, "packages", app_id)
        if os.access (pkg_repo_fp, os.X_OK):
            for f in os.listdir (pkg_repo_fp):
                tmp = re.findall('^%s_(\d+)'%(pkg_filename), f)
                if tmp:
                    pkg_revision = max (pkg_revision, int(tmp[0]))

        if pkg_revision > 0:
            pkg_fullpath = os.path.join (CHEROKEE_OWS_DIR, "packages", app_id, '%s_%d.cpk' %(pkg_filename, pkg_revision))
            CTK.cfg['tmp!market!install!local_package'] = pkg_fullpath

            Install_Log.log ("Using local repository package: %s" %(pkg_fullpath))

            box = CTK.Box()
            box += CTK.RawHTML (js=CTK.DruidContent__JS_to_goto (box.id, URL_INSTALL_SETUP))
            return box.Render().toStr()

        # Instance a Downloader
        downloader = CTK.Downloader ('package', url_download)
        downloader.bind ('stopped',  CTK.DruidContent__JS_to_close (downloader.id))
        downloader.bind ('finished', CTK.DruidContent__JS_to_goto (downloader.id, URL_INSTALL_SETUP))
        downloader.bind ('error',    CTK.DruidContent__JS_to_goto (downloader.id, URL_INSTALL_DOWNLOAD_ERROR))

        stop = CTK.Button (_('Cancel'))
        stop.bind ('click', downloader.JS_to_stop())
        buttons = CTK.DruidButtonsPanel()
        buttons += stop

        Install_Log.log ("Downloading %s" %(url_download))

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s %s</h2>' %(_("Installing"), app_name))
        cont += CTK.RawHTML ('<p>%s</p>' %(_('The application is being downloaded. Hold on tight!')))
        cont += downloader
        cont += buttons
        cont += CTK.RawHTML (js = downloader.JS_to_start())

        return cont.Render().toStr()


class Download_Error (Install_Stage):
    def __safe_call__ (self):
        app_name = CTK.cfg.get_val('tmp!market!install!app!application_name')

        Install_Log.log ("Downloading Error: %s" %(url_download))

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s %s</h2>' %(_("Installing"), app_name))
        cont += CTK.RawHTML (_("There was an error downloading the application. Please contact us if the problem persists."))

        buttons = CTK.DruidButtonsPanel()
        buttons += CTK.DruidButton_Close(_('Cancel'))
        cont += buttons

        return cont.Render().toStr()


def Exception_Handler_Apply():
    # Collect information
    info = {}
    info['log']      = Install_Log.get_full_log()
    info['user']     = OWS_Login.login_user
    info['comments'] = CTK.post['comments']
    info['platform'] = SystemInfo.get_info()
    info['cfg']      = CTK.cfg.serialize()
    info['tmp!market!install'] = CTK.cfg['tmp!market!install'].serialize()

    # Send it
    xmlrpc = XmlRpcServer (OWS_APPS_INSTALL, user=OWS_Login.login_user, password=OWS_Login.login_password)
    install_info = xmlrpc.report_exception (info)

    return CTK.cfg_reply_ajax_ok()


class Exception_Handler (CTK.Box):
    def __init__ (self, exception_str):
        CTK.Box.__init__ (self)
        self += CTK.RawHTML ('<h2 class="error-h2">%s</h2>' %(_("Internal Error")))
        self += CTK.RawHTML ('<div class="error-title">%s</div>' %(_("An internal error occurred while deploying the application")))
        self += CTK.RawHTML ('<div class="error-message">%s</div>' %(_("Information on the error has been collected so it can be reported and fixed up. Please help us improve it by sending the error information to the development team.")))

        thanks  = CTK.Box ({'class': 'error-thanks', 'style': 'display:none'})
        thanks += CTK.RawHTML (_('Thank you for your feedback! We do appreciate it.'))
        self += thanks

        comments  = CTK.Box()
        comments += CTK.RawHTML ('%s:' %(_("Comments")))
        comments += CTK.TextArea ({'name': 'comments', 'class': 'noauto error-comments'})

        submit = CTK.Submitter (URL_INSTALL_EXCEPTION)
        submit += comments
        self += submit

        report = CTK.Button (_('Report Issue'))
        cancel = CTK.DruidButton_Close (_('Cancel'))
        close  = CTK.DruidButton_Close (_('Close'), {'style': 'display:none;'})

        report.bind ('click', submit.JS_to_submit())
        submit.bind ('submit_success',
                     thanks.JS_to_show() + report.JS_to_hide() +
                     comments.JS_to_hide() + close.JS_to_show() + cancel.JS_to_hide())

        buttons = CTK.DruidButtonsPanel()
        buttons += cancel
        buttons += report
        buttons += close
        self += buttons


def _Setup_unpack():
    url_download = CTK.cfg.get_val('tmp!market!install!download')

    # has it been downloaded?
    pkg_filename = url_download.split('/')[-1]

    package_path = CTK.cfg.get_val ('tmp!market!install!local_package')
    if not package_path or not os.path.exists (package_path):
        down_entry = CTK.DownloadEntry_Factory (url_download)
        package_path = down_entry.target_path

    # Create the local directory
    target_path = os.path.join (CHEROKEE_OWS_ROOT, str(int(time.time()*100)))
    os.mkdir (target_path, 0700)
    CTK.cfg['tmp!market!install!root'] = target_path

    # Create the log file
    Install_Log.set_file (os.path.join (target_path, "install.log"))

    # Uncompress
    try:
        if sys.version_info < (2,5): # tarfile module prior to 2.5 is useless to us: http://bugs.python.org/issue1509889
            raise tarfile.CompressionError

        Install_Log.log ("Unpacking %s with Python" %(package_path))
        tar = tarfile.open (package_path, 'r:gz')
        for tarinfo in tar:
            Install_Log.log ("  %s" %(tarinfo.name))
            tar.extract (tarinfo, target_path)
        ret = {'retcode': 0}
    except tarfile.CompressionError:
        command = "gzip -dc '%s' | tar xfv -" %(package_path)
        Install_Log.log ("Unpacking %(package_path)s with the GZip binary (cd: %(target_path)s): %(command)s" %(locals()))
        ret = popen.popen_sync (command, cd=target_path)
        Install_Log.log (ret['stdout'])
        Install_Log.log (ret['stderr'])

    # Set default permission
    Install_Log.log ("Setting default permission 755 for directory %s" %(target_path))
    os.chmod (target_path, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH )

    # Remove the package
    if package_path.startswith (CHEROKEE_OWS_DIR):
        Install_Log.log ("Skipping removal of: %s" %(package_path))
    else:
        Install_Log.log ("Removing %s" %(package_path))
        os.unlink (package_path)

    return ret


class Setup (Install_Stage):
    def __safe_call__ (self):
        Install_Log.log ("Setup phase")
        app_name = CTK.cfg.get_val('tmp!market!install!app!application_name')

        box = CTK.Box()
        box += CTK.RawHTML ("<h2>%s %s</h2>" %(_("Installing"), app_name))
        box += CTK.RawHTML ("<h1>%s</h1>" %(_("Unpacking application…")))

        # Unpack
        commands = [({'function': _Setup_unpack, 'description': _("The applicaction is being unpacked…")})]
        progress = CommandProgress.CommandProgress (commands, URL_INSTALL_POST_UNPACK, props={'style': 'display:none;'})
        box += progress
        return box.Render().toStr()


class Post_unpack (Install_Stage):
    def __safe_call__ (self):
        Install_Log.log ("Post unpack commands")

        target_path = CTK.cfg.get_val ('tmp!market!install!root')
        app_name    = CTK.cfg.get_val ('tmp!market!install!app!application_name')

        # Import the Installation handler
        if os.path.exists (os.path.join (target_path, "installer.py")):
            Install_Log.log ("Passing control to installer.py")
            installer_path = os.path.join (target_path, "installer.py")
            pkg_installer = imp.load_source ('installer', installer_path)
        else:
            Install_Log.log ("Passing control to installer.pyo")
            installer_path = os.path.join (target_path, "installer.pyo")
            pkg_installer = imp.load_compiled ('installer', installer_path)

        # GUI
        box = CTK.Box()

        commands = pkg_installer.__dict__.get ('POST_UNPACK_COMMANDS',[])
        if not commands:
            box += CTK.RawHTML (js = CTK.DruidContent__JS_to_goto (box.id, URL_INSTALL_SETUP_EXTERNAL))
            return box.Render().toStr()

        box += CTK.RawHTML ("<h2>%s %s</h2>" %(_("Installing"), app_name))
        box += CTK.RawHTML ("<p>%s</p>" %(_("Setting it up…")))

        progress = CommandProgress.CommandProgress (commands, URL_INSTALL_SETUP_EXTERNAL)
        box += progress
        return box.Render().toStr()


def Install_Done_apply():
    return CTK.cfg_reply_ajax_ok()


class Install_Done (Install_Stage):
    def __safe_call__ (self):
        box = CTK.Box()

        # Automatic submit -> 'Save' button is updated.
        submit = CTK.Submitter (URL_INSTALL_DONE_APPLY)
        submit += CTK.Hidden ('foo', 'bar')
        submit.bind ('submit_success', CTK.DruidContent__JS_to_goto (box.id, URL_INSTALL_DONE_CONTENT))

        box += submit
        box += CTK.RawHTML (js=submit.JS_to_submit())

        return box.Render().toStr()


class Install_Done_Content (Install_Stage):
    def __safe_call__ (self):
        root        = CTK.cfg.get_val('tmp!market!install!root')
        app_name    = CTK.cfg.get_val('tmp!market!install!app!application_name')
        cfg_changes = CTK.cfg.get_val('tmp!market!install!cfg_previous_changes')

        # Finished
        finished_file = os.path.join (root, "finished")
        Install_Log.log ("Creating %s" %(finished_file))
        f = open (finished_file, 'w+')
        f.close()

        # Normalize CTK.cfg
        CTK.cfg.normalize ('vserver')

        # Save configuration
        box = CTK.Box()

        if not int(cfg_changes):
            CTK.cfg.save()
            Install_Log.log ("Configuration saved.")

            Cherokee.server.restart (graceful=True)
            Install_Log.log ("Server gracefully restarted.")
            box += CTK.RawHTML (js=SaveButton.ButtonSave__JS_to_deactive())

        Install_Log.log ("Finished")

        # Thank user for the install
        box += CTK.RawHTML ('<h2>%s %s</h2>' %(app_name, _("has been installed successfully")))
        box += CTK.RawHTML ("<p>%s</p>" %(_(NOTE_THANKS_P1)))

        # Save / Visit
        if int(cfg_changes):
            box += CTK.Notice ('information', CTK.RawHTML (_(NOTE_SAVE_RESTART)))

        elif Cherokee.server.is_alive():
            install_type = CTK.cfg.get_val ('tmp!market!install!target')
            nick         = CTK.cfg.get_val ('tmp!market!install!target!vserver')
            vserver_n    = CTK.cfg.get_val ('tmp!market!install!target!vserver_n')
            directory    = CTK.cfg.get_val ('tmp!market!install!target!directory')

            # Host
            if vserver_n and not nick:
                nick = CTK.cfg.get_val ("vserver!%s!nick"%(vserver_n))

            if not nick or nick.lower() == "default":
                sys_stats = SystemStats.get_system_stats()
                nick = sys_stats.hostname

            # Ports
            ports = []
            for b in CTK.cfg['server!bind'] or []:
                port = CTK.cfg.get_val ('server!bind!%s!port'%(b))
                if port:
                    ports.append (port)

            nick_port = nick
            if ports and not '80' in ports:
                nick_port = '%s:%s' %(nick, ports[0])

            # URL
            url = ''
            if install_type == 'vserver' and nick:
                url  = 'http://%s/'%(nick_port)
            elif install_type == 'directory' and vserver_n and directory:
                nick = CTK.cfg.get_val ('vserver!%s!nick'%(vserver_n))
                url  = 'http://%s%s/'%(nick_port, directory)

            if url:
                box += CTK.RawHTML ('<p>%s ' %(_("You can now visit")))
                box += CTK.LinkWindow (url, CTK.RawHTML (_('your new application')))
                box += CTK.RawHTML (' on a new window.</p>')

        box += CTK.RawHTML ("<h1>%s</h1>" %(_(NOTE_THANKS_P2)))

        buttons = CTK.DruidButtonsPanel()
        buttons += CTK.DruidButton_Close(_('Close'))
        box += buttons

        # Clean up CTK.cfg
        for k in CTK.cfg.keys('tmp!market!install'):
            if k != 'app':
                del (CTK.cfg['tmp!market!install!%s'%(k)])

        return box.Render().toStr()


CTK.publish ('^%s$'%(URL_INSTALL_WELCOME),        Welcome)
#CTK.publish ('^%s$'%(URL_INSTALL_INIT_CHECK),     Init_Check)
#CTK.publish ('^%s$'%(URL_INSTALL_PAY_CHECK),      Pay_Check)
CTK.publish ('^%s$'%(URL_INSTALL_DOWNLOAD),       Download)
CTK.publish ('^%s$'%(URL_INSTALL_SETUP),          Setup)
CTK.publish ('^%s$'%(URL_INSTALL_POST_UNPACK),    Post_unpack)
CTK.publish ('^%s$'%(URL_INSTALL_DOWNLOAD_ERROR), Download_Error)
CTK.publish ('^%s$'%(URL_INSTALL_DONE),           Install_Done)
CTK.publish ('^%s$'%(URL_INSTALL_DONE_CONTENT),   Install_Done_Content)
CTK.publish ('^%s$'%(URL_INSTALL_DONE_APPLY),     Install_Done_apply,      method="POST")
CTK.publish ('^%s$'%(URL_INSTALL_EXCEPTION),      Exception_Handler_Apply, method="POST")
