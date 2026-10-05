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
from CTK.i18n import _, N_
import validations

URL_APPLY = '/plugin/ldap/apply'
HELPS     = [('modules_validators_ldap', N_("LDAP"))]

NOTE_SERVER      = N_('LDAP server IP address.')
NOTE_PORT        = N_('LDAP server port to connect to. Default: 389')
NOTE_BIND_DOMAIN = N_('Domain sent during the LDAP authentication operation. Optional.')
NOTE_BIND_PASSWD = N_('Password to authenticate in the LDAP server.')
NOTE_BASE_DOMAIN = N_('Base domain for the web server authentications.')
NOTE_FILTER      = N_('Object filter. It can be empty.')
NOTE_USE_TLS     = N_('Enable to use secure connections between the web and LDAP servers.')
NOTE_CA_FILE     = N_('CA File for the TLS connections.')

class Plugin_ldap (Auth.PluginAuth):
    def __init__ (self, key, **kwargs):
        Auth.PluginAuth.__init__ (self, key, **kwargs)
        self.AddCommon (supported_methods=('basic',))

        table = CTK.PropsTable()
        table.Add (_("Server"),        CTK.TextCfg(f"{self.key}!server", False), _(NOTE_SERVER))
        table.Add (_("Port"),          CTK.TextCfg(f"{self.key}!port", True),  _(NOTE_PORT))
        table.Add (_("Bind Domain"),   CTK.TextCfg(f"{self.key}!bind_dn", True),  _(NOTE_BIND_DOMAIN))
        table.Add (_("Bind Password"), CTK.TextCfg(f"{self.key}!bind_pw", True),  _(NOTE_BIND_PASSWD))
        table.Add (_("Base Domain"),   CTK.TextCfg(f"{self.key}!base_dn", False), _(NOTE_BIND_DOMAIN))
        table.Add (_("Filter"),        CTK.TextCfg(f"{self.key}!filter", True),  _(NOTE_FILTER))
        table.Add (_("Use TLS"),       CTK.CheckCfgText(f"{self.key}!tls", False, _('Enabled')), _(NOTE_USE_TLS))
        table.Add (_("CA File"),       CTK.TextCfg(f"{self.key}!ca_file", True),  _(NOTE_CA_FILE))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML (f"<h2>{_('LDAP Connection')}</h2>")
        self += CTK.Indenter (submit)

        # Publish
        VALS = [(f"{self.key}!ca_file", validations.is_local_file_exists),
                (f"{self.key}!port",    validations.is_tcp_port)]

        CTK.publish (f'^{URL_APPLY}', CTK.cfg_apply_post, validation=VALS, method="POST")
