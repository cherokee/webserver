# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
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
import Auth
import validations

URL_APPLY = '/plugin/htpasswd/apply'
HELPS     = [('modules_validators_htpasswd', "Htpasswd")]

NOTE_PASSWD = N_("Full path to the Htpasswd formated password file.")

class Plugin_htpasswd (Auth.PluginAuth):
    def __init__ (self, key, **kwargs):
        Auth.PluginAuth.__init__ (self, key, **kwargs)
        self.AddCommon (supported_methods=('basic',))

        table = CTK.PropsTable()
        table.Add (_("Password File"), CTK.TextCfg("%s!passwdfile"%(self.key), False), _(NOTE_PASSWD))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML ("<h2>%s</h2>" % (_('Htpasswd Password File')))
        self += CTK.Indenter (submit)

        # Publish
        VALS = [("%s!passwdfile"%(self.key), validations.is_local_file_exists)]
        CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, validation=VALS, method="POST")
