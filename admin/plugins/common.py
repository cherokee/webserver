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
from CTK.Plugin import instance_plugin

URL_APPLY = '/plugin/common/apply'
HELPS     = [('modules_handlers_common', N_("List & Send"))]

NOTE_PATHINFO = N_("Allow extra tailing paths")
NOTE_DIRLIST  = N_("Allow to list directory contents")


class Plugin_common (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        table = CTK.PropsTable()
        table.Add (_('Allow PathInfo'),          CTK.CheckCfgText('%s!allow_pathinfo'%(key), False, _('Allow')), _(NOTE_PATHINFO))
        table.Add (_('Allow Directory Listing'), CTK.CheckCfgText('%s!allow_dirlist'%(key),  True,  _('Allow')), _(NOTE_DIRLIST))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Parsing')))
        self += CTK.Indenter (submit)
        self += instance_plugin('file',    key, show_document_root=False, symlinks=False)
        self += instance_plugin('dirlist', key, show_document_root=False, symlinks=True)

CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, method="POST")
