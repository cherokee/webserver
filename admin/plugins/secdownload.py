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
from CTK.i18n import _, N_
import validations
from CTK.Plugin import instance_plugin

URL_APPLY = '/plugin/secdownload/apply'
HELPS     = [('modules_handlers_secdownload', N_("Hidden Download"))]

NOTE_SECRET  = N_("Shared secret between the server and the script.")
NOTE_TIMEOUT = N_("How long the download will last accessible - in seconds. Default: 60.")


class Plugin_secdownload (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        table = CTK.PropsTable()
        table.Add (_('Secret'),  CTK.TextCfg(f'{key}!secret', False), _(NOTE_SECRET))
        table.Add (_('Timeout'), CTK.TextCfg(f'{key}!timeout', True), _(NOTE_TIMEOUT))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        # Streaming
        self += CTK.RawHTML (f"<h2>{_('Covering Parameters')}</h2>")
        self += CTK.Indenter (submit)

        # File
        self += instance_plugin('file', key, show_document_root=False)

        # Publish
        VALS = [(f"{key}!secret",  validations.is_not_empty),
                (f"{key}!timeout", validations.is_number_gt_0)]

        CTK.publish (f'^{URL_APPLY}', CTK.cfg_apply_post, validation=VALS, method="POST")
