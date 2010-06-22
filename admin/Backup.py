# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega
#      Taher Shihadeh
#
# Copyright (C) 2010 Alvaro Lopez Ortega
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

import copy
import time
import socket

import CTK
import Login
import XMLServerDigest

OWS_BACKUP      = 'http://www.octality.com/api/backup/'
OWS_BACKUP      = 'http://127.0.0.1:7001/api/backup/'

URL_BASE          = '/backup'
URL_DRUID_NOTE    = '%s/druid/note'    %(URL_BASE)
URL_DRUID_FAIL    = '%s/druid/fail'    %(URL_BASE)
URL_DRUID_SUCCESS = '%s/druid/success' %(URL_BASE)
SAVE_APPLY        = '%s/upload/apply'  %(URL_BASE)
RESTORE_APPLY     = '%s/download/apply'%(URL_BASE)

NOTE_SAVE_H1      = N_('Remote Configuration Back Up')
NOTE_SAVE_P1      = N_('Your current configuration is about to be backed up. By submitting the form it will be stored at a remote location and will be made available to you for future use.')
NOTE_SAVE_NOTES   = N_('You can store some annotations along the configuration that is about to be saved.')
NOTE_SAVE_FAIL_H1 = N_('Could not Upload the Configuration File')
NOTE_SAVE_FAIL_P1 = N_('An error occurred while upload your configuration file. Please try again later.')
NOTE_SAVE_OK_H1   = N_('Ok H1')       # fixme
NOTE_SAVE_OK_P1   = N_('bla bla bla') # fixme

NOTE_RESTORE_H1   = N_('Remote Configuration Restoration')
NOTE_RESTORE_P1   = N_('Select a configuration state to restore. Submitting the form will download and restore a previous configuration. You might want to save the current one before you proceed.')
NOTE_RESTORE_ASK  = N_('You are about to replace your current configuration')
NOTE_RESTORE_NO   = N_('No configurations could be retireved. It seems you have not uploaded any configuration yet.')


#
# Apply
#

class Apply:
    class Save:
        def __call__ (self):
            comment = CTK.post.get_val('comment')
            config  = CTK.cfg.serialize()

            try:
                xmlrpc = XMLServerDigest.XmlRpcServer (OWS_BACKUP, Login.login_user, Login.login_password)
                ret    = xmlrpc.upload (config, comment)
            except socket.error, (value, message):
                CTK.cfg['tmp!backup!save!type']  = 'socket'
                CTK.cfg['tmp!backup!save!error'] = message
                return {'ret': 'error'}
            except Exception, e:
                CTK.cfg['tmp!backup!save!type']  = 'general'
                CTK.cfg['tmp!backup!save!error'] = str(e)
                return {'ret': 'error'}

            return {'ret': ret}

    class Restore:
        def __call__ (self):
            version = CTK.post.get_val('version')
            try:
                xmlrpc  = XMLServerDigest.XmlRpcServer (OWS_BACKUP, Login.login_user, Login.login_password)
                cfg_str = xmlrpc.download (version)
                cfg     = CTK.Config()

                cfg._parse (cfg_str)
                CTK.cfg.root = copy.deepcopy(cfg.root)
            except Exception, e:
                return {'ret': 'error', 'error': str(e)}

            return CTK.cfg_reply_ajax_ok()


class Save_Config_Button (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'backup-save'})

        # Druid
        druid  = CTK.Druid (CTK.RefreshableURL())
        dialog = CTK.Dialog ({'title': _(NOTE_SAVE_H1), 'width': 550})
        dialog += druid
        druid.bind ('druid_exiting', dialog.JS_to_close())

        # Trigger button
        button = CTK.Button(_('Back-up Configuration...'))
        button.bind ('click', druid.JS_to_goto('"%s"'%(URL_DRUID_NOTE)) + dialog.JS_to_show())

        self += dialog
        self += button


#
# Save Druid
#

class Backup_Druid_Note:
    def __call__ (self):
        # Form
        submit = CTK.Submitter (SAVE_APPLY)
        submit += CTK.TextArea ({'name': 'comment', 'class': 'noauto', 'class': 'backup-notes-textarea optional'})
        submit.bind ('submit_success', CTK.DruidContent__JS_to_goto (submit.id, URL_DRUID_SUCCESS))
        submit.bind ('submit_fail',    CTK.DruidContent__JS_to_goto (submit.id, URL_DRUID_FAIL))

        # Buttons
        panel = CTK.DruidButtonsPanel()
        panel += CTK.DruidButton_Submit (_('Back Up'))
        panel += CTK.DruidButton_Close  (_('Cancel'))

        # Layout
        content = CTK.Container()
        content += CTK.RawHTML (_(NOTE_SAVE_P1))
        content += CTK.RawHTML ("<h3>%s</h3>" %(_("Notes")))
        content += submit
        content += panel

        return content.Render().toStr()

class Backup_Druid_Fail:
    def __call__ (self):
        panel = CTK.DruidButtonsPanel()
        panel += CTK.DruidButton_Close  (_('Close'))

        # Layout
        content = CTK.Container()
        content += CTK.RawHTML('<h1>%s</h1>' %(NOTE_SAVE_FAIL_H1))
        content += CTK.RawHTML('<p>%s</p>'   %(NOTE_SAVE_FAIL_P1))
        content += CTK.RawHTML('<p><pre>%s</pre></p>' %(CTK.cfg.get_val('tmp!backup!save!error')))
        content += panel
        return content.Render().toStr()

class Backup_Druid_Success:
    def __call__ (self):
        panel = CTK.DruidButtonsPanel()
        panel += CTK.DruidButton_Close  (_('Close'))

        # Layout
        content = CTK.Container()
        content += CTK.RawHTML('<h1>%s</h1>' %(NOTE_SAVE_OK_H1))
        content += CTK.RawHTML('<p>%s</p>'   %(NOTE_SAVE_OK_P1))
        content += panel
        return content.Render().toStr()


#
# Restore Druid
#

class Restore_Config_Form (CTK.Container):
    def __init__ (self, configs):
        CTK.Container.__init__ (self)

        table = CTK.Table ({'class': 'backup-restore-table'})

        table += [CTK.RawHTML(x) for x in ('', _('Back Up Date'), _('Annotations'))]
        table.set_header(1)

        form  = CTK.Submitter (RESTORE_APPLY)
        form += table

        for cfg in configs:
            version, date, annotation = cfg
            widget = CTK.Radio ({'name':'version', 'value':version, 'group':'config_version', 'class': 'noauto'})
            table += [widget, CTK.RawHTML('<em>%s</em>'%(time.asctime(date.timetuple()))), CTK.RawHTML(annotation)]

        self += CTK.RawHTML (_(NOTE_RESTORE_P1))
        self += form


class Restore_Config_Button (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'backup-restore'})

        configs = self.__get_configs()
        dialog  = CTK.Dialog ({'title': _(NOTE_RESTORE_H1), 'width': 500})

        if configs:
            dialog += Restore_Config_Form (configs)
            dialog.AddButton (_('Restore'), dialog.JS_to_trigger('submit'))
        else:
            dialog += CTK.Notice (content = CTK.RawHTML (_(NOTE_RESTORE_NO)))

        dialog.AddButton (_('Close'), "close")

        button = CTK.Button(_('Restore configuration'))
        button.bind ('click', dialog.JS_to_show())
        dialog.bind ('submit_success', dialog.JS_to_close())
        dialog.bind ('submit_success', self.JS_to_trigger('submit_success'));

        self += button
        self += dialog


    def __get_configs (self):
        try:
            xmlrpc  = XMLServerDigest.XmlRpcServer (OWS_BACKUP, Login.login_user, Login.login_password)
            configs = xmlrpc.list_extended ()
            configs.reverse()
        except Exception,e:
            configs = None
            print str(e)

        return configs


CTK.publish (r'^%s$'%(SAVE_APPLY),    Apply.Save,    method="POST")
CTK.publish (r'^%s$'%(RESTORE_APPLY), Apply.Restore, method="POST")

CTK.publish (r'^%s$'%(URL_DRUID_NOTE),    Backup_Druid_Note)
CTK.publish (r'^%s$'%(URL_DRUID_FAIL),    Backup_Druid_Fail)
CTK.publish (r'^%s$'%(URL_DRUID_SUCCESS), Backup_Druid_Success)
