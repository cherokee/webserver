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
import Cherokee
import validations

from util import *
from consts import *
from configured import *

URL_APPLY = '/plugin/auth/apply'

NOTE_METHODS = N_('Allowed HTTP Authentication methods.')
NOTE_REALM   = N_('Name associated with the protected resource.')
NOTE_USERS   = N_('User filter. List of allowed users.')


class PluginAuth (CTK.Plugin):
    def __init__ (self, key, **kwargs):
        CTK.Plugin.__init__ (self, key)
        self.supported_methos = []

    def AddCommon (self, supported_methods):
        assert type(supported_methods) is tuple

        if len(supported_methods) > 1:
            methods = trans_options (VALIDATOR_METHODS)
        else:
            methods = trans_options (filter (lambda x: x[0] in supported_methods, VALIDATOR_METHODS))

        table = CTK.PropsTable()
        table.Add (_("Methods"), CTK.ComboCfg("%s!methods"%(self.key), trans_options(methods), {'id': 'auth_method'}), _(NOTE_METHODS))
        table.Add (_("Realm"),   CTK.TextCfg("%s!realm" %(self.key), False), _(NOTE_REALM))
        table.Add (_("Users"),   CTK.TextCfg("%s!users" %(self.key), True),  _(NOTE_USERS))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Authentication Details')))
        self += CTK.Indenter(submit)

        # Publish
        VALS = [("%s!users"%(self.key), validations.is_safe_id_list)]
        CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, validation=VALS, method="POST")
