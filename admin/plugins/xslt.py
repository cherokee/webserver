# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
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

import CTK
import Handler

from util import *
from consts import *

URL_APPLY = '/plugin/xslt/apply'
HELPS     = [('modules_handlers_xslt', N_("XSLT"))]

OPTIONS = [
    ('',          N_("None")),
    ('novalid',   N_("No valid")),
    ('nodtdattr', N_("No DTD Attribute"))
]

NOTE_OPTIONS       = N_('How the XML should be parsed.')
NOTE_NODICT        = N_('Disable dictionary.')
NOTE_CATALOGS      = N_('Enable SGML Catalog Files.')
NOTE_XINCLUDE      = N_('Enable XInclude.')
NOTE_XINCLUDESTYLE = N_('Enable XInclude style.')
NOTE_CONTENTTYPE   = N_('The Content-Type the file should be send as.')
NOTE_STYLESHEET    = N_('The XSL stylesheet to be used for any transformation.')


class Plugin_xslt (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        table = CTK.PropsTable()
        table.Add (_("Options"), CTK.ComboCfg('%s!options'%(key), trans_options(OPTIONS)), _(NOTE_OPTIONS))
	table.Add (_("No dictionary"), CTK.CheckCfgText("%s!nodict"%(self.key), True, ''), _(NOTE_NODICT))
	table.Add (_("Catalogs"), CTK.CheckCfgText("%s!catalogs"%(self.key), True, ''), _(NOTE_CATALOGS))
	table.Add (_("XInclude"), CTK.CheckCfgText("%s!xinclude"%(self.key), True, _('Enabled')), _(NOTE_XINCLUDE))
	table.Add (_("XincludeStyle"), CTK.CheckCfgText("%s!xincludestyle"%(self.key), True, _('Enabled')), _(NOTE_XINCLUDESTYLE))
	table.Add (_("Content-Type"), CTK.TextCfg ('%s!contenttype'%(key), True), _(NOTE_CONTENTTYPE))
	table.Add (_("Stylesheet"), CTK.TextCfg ('%s!stylesheet'%(key), True), _(NOTE_STYLESHEET))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML ("<h2>%s</h2>" %(_('XML Parser Settings')))
        self += CTK.Indenter (submit)

	VALS = [("%s!stylesheet"%(self.key), validations.is_local_file_exists)]
	CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, validation=VALS, method="POST")
