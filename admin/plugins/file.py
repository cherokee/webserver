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

URL_APPLY = '/plugin/file/apply'
HELPS     = [('modules_handlers_file', _("Static Content"))]

NOTE_IO_CACHE = N_('Enables an internal I/O cache that improves performance.')


class Plugin_file (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        symlinks = kwargs.pop('symlinks', True)

        table = CTK.PropsTable()
        table.Add (_("Use I/O cache"), CTK.CheckCfgText("%s!iocache"%(self.key), True, _('Enabled')), _(NOTE_IO_CACHE))
        if symlinks:
            table.Add (_('Allow symbolic links'), CTK.CheckCfgText("%s!symlinks"%(self.key), True,  _('Allow')), '')

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML ('<h2>%s</h2>' % (_('File Sending')))
        self += CTK.Indenter (submit)

CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, method="POST")
