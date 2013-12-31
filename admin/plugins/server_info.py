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
import Handler

from util import *
from consts import *

URL_APPLY = '/plugin/server_info/apply'
HELPS     = [('modules_handlers_server_info', N_("Server Information"))]

OPTIONS = [
    ('normal',             N_("Server Information")),
    ('just_about',         N_("Only version information")),
    ('connection_details', N_("Server Information + Connections"))
]

NOTE_INFORMATION = N_('Which information should be shown.')


class Plugin_server_info (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        kwargs['show_document_root'] = False
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        table = CTK.PropsTable()
        table.Add (_("Show Information"), CTK.ComboCfg('%s!type'%(key), trans_options(OPTIONS)), _(NOTE_INFORMATION))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML ("<h2>%s</h2>" %(_('Privacy Settings')))
        self += CTK.Indenter (submit)

CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, method="POST")
