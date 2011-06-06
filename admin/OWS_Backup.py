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
import OWS_Login
import XMLServerDigest

from configured import *

OWS_BACKUP = 'http://www.octality.com/api/v%s/backup/' %(OWS_API_VERSION)

URL_BASE            = '/backup'
URL_SAVE_NOTE       = '%s/save/note'       %(URL_BASE)
URL_SAVE_FAIL       = '%s/save/fail'       %(URL_BASE)
URL_SAVE_SUCCESS    = '%s/save/success'    %(URL_BASE)
URL_SAVE_APPLY      = '%s/save/apply'      %(URL_BASE)
URL_RESTORE_NOTE    = '%s/restore/note'    %(URL_BASE)
URL_RESTORE_FAIL    = '%s/restore/fail'    %(URL_BASE)
URL_RESTORE_SUCCESS = '%s/restore/success' %(URL_BASE)
URL_RESTORE_APPLY   = '%s/restore/apply'   %(URL_BASE)

NOTE_SAVE_H2      = N_('Remote Configuration Back Up')
NOTE_SAVE_P1      = N_('Your current configuration is about to be backed up.')
NOTE_SAVE_P2      = N_('By submitting the form it will be stored at a remote location and will be made available to you for future use.')
NOTE_SAVE_FAIL_H2 = N_('Could not Upload the Configuration File')
NOTE_SAVE_FAIL_P1 = N_('An error occurred while uploading your configuration file. Please try again later.')
NOTE_SAVE_OK_H2   = N_('Configuration File Successfully uploaded')
NOTE_SAVE_OK_P1   = N_('Your configuration file has been saved. From now on the back-up will be available for restoration.')

NOTE_RESTORE_H2   = N_('Remote Configuration Restoration')
NOTE_RESTORE_P1   = N_('Select a configuration state to restore. Submitting the form will download and restore a previous configuration.')
NOTE_RESTORE_P2   = N_('You might want to save the current one before you proceed.')
NOTE_RESTORE_NO   = N_('No configurations could be retireved. It seems you have not uploaded any configuration yet.')

NOTE_RESTORE_ERROR_H2 = N_('Could not retrieve the configuration file list')
NOTE_RESTORE_ERROR_P1 = N_('An error occurred while trying to fetch the configuration file list from the remote server.')
NOTE_RESTORE_FAIL_H2  = N_('Could not restore the selected configuration file')
NOTE_RESTORE_FAIL_P1  = N_('An error occurred while trying to restore a remotely-stored configuration file.')
NOTE_RESTORE_OK_H2    = N_('The Configuration File was restored successfully')
NOTE_RESTORE_OK_P1    = N_('The configuration has been rolled back to a previous version.')
NOTE_RESTORE_OK_P2_1  = N_('Please, click the Save button if you want to commit the changes.')
NOTE_RESTORE_OK_P2_2  = N_('Your current Configuration file matches the restored version.')


#
# Apply
#

class Apply:
    class Save:
        def __call__ (self):
            comment = CTK.post.get_val('comment')
            config  = CTK.cfg.serialize()

            del (CTK.cfg['tmp!backup!save'])

            try:
                xmlrpc = XMLServerDigest.XmlRpcServer (OWS_BACKUP, OWS_Login.login_user, OWS_Login.login_password)
                ret    = xmlrpc.upload (CTK.util.to_unicode(config),
                                        CTK.util.to_unicode(comment))
            except socket.error, (value, message):
                CTK.cfg['tmp!backup!save!type']  = 'socket'
                CTK.cfg['tmp!backup!save!error'] = message
                return {'ret': 'error', 'error': message}
            except Exception, e:
                CTK.cfg['tmp!backup!save!type']  = 'general'
                CTK.cfg['tmp!backup!save!error'] = str(e)
                return {'ret': 'error', 'error': str(e)}

            return {'ret': ret}

    class Restore:
        def __call__ (self):
            version = CTK.post.get_val('version')

            del (CTK.cfg['tmp!backup!restore'])

            try:
                xmlrpc  = XMLServerDigest.XmlRpcServer (OWS_BACKUP, OWS_Login.login_user, OWS_Login.login_password)
                cfg_str = xmlrpc.download (version)
                cfg     = CTK.Config()

                cfg._parse (cfg_str)
                CTK.cfg.root = copy.deepcopy(cfg.root)
            except socket.error, (value, message):
                CTK.cfg['tmp!backup!restore!type']  = 'socket'
                CTK.cfg['tmp!backup!restore!error'] = message
                return {'ret': 'error', 'error': message}
            except Exception, e:
                CTK.cfg['tmp!backup!restore!type']  = 'general'
                CTK.cfg['tmp!backup!restore!error'] = str(e)
                return {'ret': 'error', 'error': str(e)}

            return CTK.cfg_reply_ajax_ok()


class Save_Config_Button (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'backup-save'})

        # Druid
        druid  = CTK.Druid (CTK.RefreshableURL())
        dialog = CTK.Dialog ({'title': _(NOTE_SAVE_H2), 'width': 480})
        dialog += druid
        druid.bind ('druid_exiting', dialog.JS_to_close())

        # Trigger button
        link = CTK.Link ("#", CTK.RawHTML(_('Back up…')))
        link.bind ('click', druid.JS_to_goto('"%s"'%(URL_SAVE_NOTE)) + dialog.JS_to_show())

        self += dialog
        self += link


#
# Save Druid
#

class Backup_Save_Note:
    def __call__ (self):
        # Form
        submit = CTK.Submitter (URL_SAVE_APPLY)
        submit += CTK.TextArea ({'name': 'comment', 'class': 'noauto backup-notes-textarea optional', 'style': 'width: 436px; height: 48px; margin-top: 8px;' })
        submit.bind ('submit_success', CTK.DruidContent__JS_to_goto (submit.id, URL_SAVE_SUCCESS))
        submit.bind ('submit_fail',    CTK.DruidContent__JS_to_goto (submit.id, URL_SAVE_FAIL))

        # Buttons
        panel = CTK.DruidButtonsPanel()
        panel += CTK.DruidButton_Close  (_('Cancel'))
        panel += CTK.DruidButton_Submit (_('Back Up'))

        # Layout
        content = CTK.Container()
        content += CTK.RawHTML ('<p>%s</p>'%(_(NOTE_SAVE_P1)))
        content += CTK.RawHTML ('<p>%s</p>'%(_(NOTE_SAVE_P2)))
        box      = CTK.Box()
        box     += CTK.RawHTML ("<strong>%s</strong>" %(_("Notes")))
        box     += submit
        content += box
        content += panel

        return content.Render().toStr()

class Backup_Save_Fail:
    def __call__ (self):
        panel = CTK.DruidButtonsPanel()
        panel += CTK.DruidButton_Close  (_('Close'))

        # Layout
        content = CTK.Container()
        content += CTK.RawHTML('<h2>%s</h2>' %(_(NOTE_SAVE_FAIL_H2)))
        content += CTK.RawHTML('<p>%s</p>'   %(_(NOTE_SAVE_FAIL_P1)))
        content += CTK.RawHTML('<p><pre>%s</pre></p>' %(CTK.escape_html (CTK.cfg.get_val('tmp!backup!save!error',''))))
        content += panel
        return content.Render().toStr()

class Backup_Save_Success:
    def __call__ (self):
        panel = CTK.DruidButtonsPanel()
        panel += CTK.DruidButton_Close  (_('Close'))

        # Layout
        content = CTK.Container()
        content += CTK.RawHTML('<h2>%s</h2>' %(_(NOTE_SAVE_OK_H2)))
        content += CTK.RawHTML('<p>%s</p>'   %(_(NOTE_SAVE_OK_P1)))
        content += panel
        return content.Render().toStr()


#
# Restore Druid
#

class Restore_Config_Form (CTK.Submitter):
    def __init__ (self, configs):
        CTK.Submitter.__init__ (self, URL_RESTORE_APPLY)

        # Build table
        table = CTK.Table ({'class': 'backup-restore-table'})
        table += [CTK.RawHTML(x) for x in ('', _('Back Up Date'), _('Annotations'))]
        table.set_header(1)

        # Populate it
        for cfg in configs:
            version, date, annotation = cfg

            # Parse time
            iso8601_date =  str(date)
            time_tuple   = time.strptime (iso8601_date.replace("-", ""), "%Y%m%dT%H:%M:%S")

            # Add the row
            widget = CTK.Radio ({'name':'version', 'value':version, 'group':'config_version', 'class': 'noauto'})
            table += [widget, CTK.RawHTML('<em>%s</em>'%(time.asctime(time_tuple))), CTK.RawHTML(annotation)]

        self += table


class Backup_Restore_Note:
    def __call__ (self):
        configs = _get_configs()
        panel   = CTK.DruidButtonsPanel()
        content = CTK.Container()

        error = CTK.cfg.get_val('tmp!backup!retrieve!error', '')
        if error:
            content += CTK.RawHTML('<h2>%s</h2>' %(_(NOTE_RESTORE_ERROR_H2)))
            content += CTK.RawHTML('<p>%s</p>'   %(_(NOTE_RESTORE_ERROR_P1)))
            content += CTK.RawHTML('<p><pre>%s</pre></p>' %(CTK.escape_html(error)))
            content += panel
            panel += CTK.DruidButton_Close  (_('Close'))

        elif not configs:
            # Content
            content += CTK.Notice (content = CTK.RawHTML (_(NOTE_RESTORE_NO)))
            content += panel

            # Buttons
            panel += CTK.DruidButton_Close  (_('Close'))

        else:
            form = Restore_Config_Form (configs)
            form.bind ('submit_success', CTK.DruidContent__JS_to_goto (form.id, URL_RESTORE_SUCCESS))
            form.bind ('submit_fail',    CTK.DruidContent__JS_to_goto (form.id, URL_RESTORE_FAIL))

            # Content
            content += CTK.RawHTML ('<p>%s</p>'%(_(NOTE_RESTORE_P1)))
            content += CTK.RawHTML ('<p>%s</p>'%(_(NOTE_RESTORE_P2)))
            content += form
            content += panel

            # Buttons
            panel += CTK.DruidButton_Close  (_('Cancel'))
            panel += CTK.DruidButton_Submit (_('Restore'))

        return content.Render().toStr()

class Backup_Restore_Fail:
    def __call__ (self):
        panel = CTK.DruidButtonsPanel()
        panel += CTK.DruidButton_Close  (_('Close'))

        content = CTK.Container()
        content += CTK.RawHTML('<h2>%s</h2>' %(_(NOTE_RESTORE_FAIL_H2)))
        content += CTK.RawHTML('<p>%s</p>'   %(_(NOTE_RESTORE_FAIL_P1)))
        content += CTK.RawHTML('<p><pre>%s</pre></p>' %(CTK.escape_html (CTK.cfg.get_val('tmp!backup!restore!error',''))))
        content += panel
        return content.Render().toStr()

class Backup_Restore_Success:
    def __call__ (self):
        panel = CTK.DruidButtonsPanel()
        panel += CTK.DruidButton_Close  (_('Close'))

        content = CTK.Container()
        content += CTK.RawHTML('<h2>%s</h2>' %(_(NOTE_RESTORE_OK_H2)))
        content += CTK.RawHTML('<p>%s</p>'   %(_(NOTE_RESTORE_OK_P1)))

        if CTK.cfg.has_changed():
            content += CTK.RawHTML('<p>%s</p>' %(_(NOTE_RESTORE_OK_P2_1)))
        else:
            content += CTK.RawHTML('<p>%s</p>' %(_(NOTE_RESTORE_OK_P2_2)))

        content += panel
        return content.Render().toStr()

class Restore_Config_Button (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'backup-restore'})

        # Druid
        druid  = CTK.Druid (CTK.RefreshableURL())
        dialog = CTK.Dialog ({'title': _(NOTE_RESTORE_H2), 'width': 480})
        dialog += druid

        # Trigger button
        link = CTK.Link ("#", CTK.RawHTML(_('Restore…')))
        link.bind ('click', druid.JS_to_goto('"%s"'%(URL_RESTORE_NOTE)) + dialog.JS_to_show())

        self += dialog
        self += link


#
# Utils
#

def _get_configs():
    del (CTK.cfg['tmp!backup!retrieve'])

    try:
        xmlrpc  = XMLServerDigest.XmlRpcServer (OWS_BACKUP, OWS_Login.login_user, OWS_Login.login_password)
        configs = xmlrpc.list_extended ()
        configs.reverse()
    except socket.error, (value, message):
        CTK.cfg['tmp!backup!retrieve!type']  = 'socket'
        CTK.cfg['tmp!backup!retrieve!error'] = message
        return None
    except Exception, e:
        CTK.cfg['tmp!backup!retrieve!type']  = 'general'
        CTK.cfg['tmp!backup!retrieve!error'] = str(e)
        return None

    return configs


CTK.publish (r'^%s$'%(URL_SAVE_NOTE),       Backup_Save_Note)
CTK.publish (r'^%s$'%(URL_SAVE_FAIL),       Backup_Save_Fail)
CTK.publish (r'^%s$'%(URL_SAVE_SUCCESS),    Backup_Save_Success)
CTK.publish (r'^%s' %(URL_SAVE_APPLY),      Apply.Save,    method="POST")

CTK.publish (r'^%s$'%(URL_RESTORE_NOTE),    Backup_Restore_Note)
CTK.publish (r'^%s$'%(URL_RESTORE_FAIL),    Backup_Restore_Fail)
CTK.publish (r'^%s$'%(URL_RESTORE_SUCCESS), Backup_Restore_Success)
CTK.publish (r'^%s' %(URL_RESTORE_APPLY),   Apply.Restore, method="POST")
