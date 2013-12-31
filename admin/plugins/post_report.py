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

URL_APPLY = '/plugin/post_report/apply'
HELPS     = [('modules_handlers_post_report', _("POST Report"))]

NOTE_LANGUAGES   = N_("Target language for which the information will be encoded. (Default: JSON)")
WARNING_NO_TRACK = N_('The <a href="/general">Upload tracking mechanism</a> must be enabled for this handler to work.')

class Plugin_post_report (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        kwargs['show_document_root'] = False
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        # Ensure POST track is enabled
        if not CTK.cfg.get_val('server!post_track'):
            self += CTK.Notice ('warning', CTK.RawHTML (_(WARNING_NO_TRACK)))

        # Properties
        table = CTK.PropsTable()
        table.Add (_('Target language'), CTK.ComboCfg('%s!lang'%(key), trans_options(DWRITER_LANGS)), _(NOTE_LANGUAGES))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Indenter (table)

        self += submit

CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, method="POST")
