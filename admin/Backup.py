# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
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
import CTK
import Login
import XMLServerDigest

OWS_BACKUP    = 'http://www.octality.com/api/backup/'
BASE_URL      = '/backup'
SAVE_APPLY    = '%s/upload/apply'  %(BASE_URL)
RESTORE_APPLY = '%s/download/apply'%(BASE_URL)

NOTE_COMMENT   = N_('Annotations about the configuration that is about to be saved.')
NOTE_NO_CONFIG = N_('No configurations could be retireved. Please try again later if you are sure you have remote backups.')
NOTE_RESTORE   = N_('Select a configuration version to be restored')


def Save_Apply ():
    comment = CTK.post.get_val('comment')
    config  = CTK.cfg.serialize()

    try:
        xmlrpc = XMLServerDigest.XmlRpcServer (OWS_BACKUP, Login.login_user, Login.login_password)
        ret    = xmlrpc.upload (config, comment)
    except:
        ret    = 'error'

    return {'ret': ret}


def Restore_Apply ():
    version = CTK.post.get_val('version')

    try:
        xmlrpc  = XMLServerDigest.XmlRpcServer (OWS_BACKUP, Login.login_user, Login.login_password)

        cfg_str = xmlrpc.download (version)
        cfg     = CTK.Config()

        cfg._parse (cfg_str)
        cfg.root_orig = copy.deepcopy(CTK.cfg.root)
        CTK.cfg       = cfg
    except:
        return {'ret': 'error'}

    return {'ret': 'ok'}


class Save_Form (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        table = CTK.PropsTable()
        table.Add (_('Notes'), CTK.TextArea({'name': 'comment', 'class': 'noauto'}), _(NOTE_COMMENT))
        submit = CTK.Submitter(SAVE_APPLY)
        submit += table
        self += submit


class Save_Config_Button (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'backup-save'})

        # Add New
        dialog = CTK.Dialog ({'title': _('Remote configuration backup'), 'width': 500})
        dialog.AddButton (_('Upload'), dialog.JS_to_trigger('submit'))
        dialog.AddButton (_('Close'), "close")
        dialog += Save_Form()

        button = CTK.Button(_('Upload config'))
        button.bind ('click', dialog.JS_to_show())
        dialog.bind ('submit_success', dialog.JS_to_close())
        dialog.bind ('submit_success', self.JS_to_trigger('submit_success'));

        self += button
        self += dialog


class Version_Table (CTK.Table):
    def __init__ (self, configs):
        CTK.Table.__init__ (self, {'class': 'backup-table'})

        self += [CTK.RawHTML(x) for x in (_('Version'), _('Date'), _('Annotations'))]
        self.set_header(1)
        num = 2
        for cfg in configs:
            self += [CTK.RawHTML (str(x)) for x in cfg]
            num += 1


class Version_Form (CTK.PropsTableAuto):
    def __init__ (self, configs):
        CTK.PropsTableAuto.__init__ (self, RESTORE_APPLY)

        options = [(_('Choose'), '')] + [(str(cfg[0]), str(cfg[0])) for cfg in configs]

        self.Add (_('Version'), CTK.Combobox ({'name':'version'}, options), _(NOTE_RESTORE))


class Restore_Config (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'backup-restore'})

        Login.login_user='taher'
        Login.login_password='12314'
        try:
            xmlrpc  = XMLServerDigest.XmlRpcServer (OWS_BACKUP, Login.login_user, Login.login_password)
            configs = xmlrpc.list_extended ()
            self   += Version_Table (configs)
            self   += Version_Form  (configs)
        except:
            self   += CTK.Notice (content = CTK.RawHTML (_(NOTE_NO_CONFIG)))

CTK.publish (r'^%s$'%(SAVE_APPLY),    Save_Apply,    method="POST")
CTK.publish (r'^%s$'%(RESTORE_APPLY), Restore_Apply, method="POST")
