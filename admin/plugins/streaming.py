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
import validations
from CTK.Plugin import instance_plugin

URL_APPLY = '/plugin/streaming/apply'
HELPS     = [('modules_handlers_streaming', N_("Audio/Video Streaming"))]

NOTE_RATE        = N_('Figure the bit rate of the media file, and limit the bandwidth to it.')
NOTE_RATE_FACTOR = N_('Factor by which the bandwidth limit will be increased. Default: 0.1')
NOTE_RATE_BOOST  = N_('Number of seconds to stream before setting the bandwidth limit. Default: 5.')


class Plugin_streaming (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        rate = CTK.CheckCfgText("%s!rate"%(key), True, _('Enabled'))

        table = CTK.PropsTable()
        table.Add (_("Auto Rate"), rate, _(NOTE_RATE))
        table.Add (_("Speedup Factor"), CTK.TextCfg("%s!rate_factor"%(key), True), _(NOTE_RATE_FACTOR))
        table.Add (_("Initial Boost"),  CTK.TextCfg("%s!rate_boost"%(key), True), _(NOTE_RATE_BOOST))

        rate.bind ('change',
                  """if ($('#%(rate)s :checkbox')[0].checked) {
                           $('#%(row1)s').show();
                           $('#%(row2)s').show();
                     } else {
                           $('#%(row1)s').hide();
                           $('#%(row2)s').hide();
                  }""" %({'rate': rate.id, 'row1': table[1].id, 'row2': table[2].id}))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        # Streaming
        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Audio/Video Streaming')))
        self += CTK.Indenter (submit)

        # File
        self += instance_plugin('file',    key, show_document_root=False)

        # Publish
        VALS = [("%s!rate_factor"%(key), validations.is_float),
                ("%s!rate_boost"%(key),  validations.is_number_gt_0)]

        CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, validation=VALS, method="POST")
