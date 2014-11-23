# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#      Stefan de Konink <stefan@konink.de>
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
import Cherokee
import validations
import Balancer

from util import *
from consts import *

NOTE_TIMEOUT = N_("A value in seconds after which we don't wait for the rendering anymore.")
NOTE_EXPIRATION_TIME = N_("""How long the older tile should be cached.<br />
The <b>m</b>, <b>h</b>, <b>d</b> and <b>w</b> suffixes are allowed for minutes, hours, days, and weeks. Eg: 10m.
""")
NOTE_RERENDER = N_("If the file is older than this modification time, we will send a render request.")

URL_APPLY = '/plugin/tile/apply'
HELPS     = []

VALIDATIONS = [
	("vserver![\d]+!rule![\d]+!handler!timeout",    validations.is_number),
	("vserver![\d]+!rule![\d]+!handler!expiration", validations.is_time)
]

class Plugin_tile (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        kwargs['show_document_root'] = True
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        # GUI
        table = CTK.PropsAuto (URL_APPLY)
        table.Add (_("Static Tile Expiration"), CTK.TextCfg('%s!expiration'%(key), True), _(NOTE_EXPIRATION_TIME))
        table.Add (_("Force Rerender"), CTK.TextCfg('%s!rerender'%(key), True), _(NOTE_RERENDER))
        table.Add (_("Render Timeout"), CTK.TextCfg('%s!timeout'%(key), True), _(NOTE_TIMEOUT))
	self += CTK.Indenter (table)

        # Load Balancing
        modul = CTK.PluginSelector('%s!balancer'%(key), trans_options(Cherokee.support.filter_available (BALANCERS)))
        table = CTK.PropsTable()
        table.Add (_("Balancer"), modul.selector_widget, _(Balancer.NOTE_BALANCER))

        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Renderd Balancing')))
        self += CTK.Indenter (table)
        self += modul

CTK.publish ('^%s$'%(URL_APPLY), CTK.cfg_apply_post, validation=VALIDATIONS, method="POST")
