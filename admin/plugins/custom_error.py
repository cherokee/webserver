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
from consts import *

URL_APPLY   = '/plugin/custom_error/apply'
NOTE_ERRORS = N_('HTTP Error that you be used to reply the request.')

HELPS = [('modules_handlers_custom_error', N_("HTTP Custom Error"))]


class Plugin_custom_error (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        kwargs['show_document_root'] = False
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        error_codes = [('', _('Choose'))] + ERROR_CODES
        if CTK.cfg.get_val('%s!error'%(key)):
            error_codes.pop(0)

        table = CTK.PropsTable()
        table.Add (_("HTTP Error"), CTK.ComboCfg('%s!error'%(key), error_codes), _(NOTE_ERRORS))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML ("<h2>%s</h2>" %(_('Custom Error')))
        self += CTK.Indenter (submit)

CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, method="POST")
