# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#      Stefan de Konink <stefan@konink.de>
#
# Copyright (C) 2013 Alvaro Lopez Ortega
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
import consts

URL_APPLY = '/plugin/zeromq/apply'

NOTE_ENDPOINT   = N_("Endpoint to push to.")
NOTE_IO_THREADS = N_("Number of I/O-threads to use for ZeroMQ.")
NOTE_LEVEL      = N_("Compression Level from 0 to 9, where 0 is no compression, 1 best speed, and 9 best compression.")

class Plugin_zeromq (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        kwargs['show_document_root'] = False
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        table = CTK.PropsTable()
        table.Add (_('Endpoint'), CTK.TextCfg('%s!endpoint'%(key), False),  _(NOTE_ENDPOINT))
        table.Add (_('I/O Threads'), CTK.TextCfg('%s!io_threads'%(key), True),  _(NOTE_IO_THREADS))
	table.Add (_("Compression Level"), CTK.ComboCfg('%s!encoder!compression_level'%(key), consts.COMPRESSION_LEVELS), _(NOTE_LEVEL))
        submit = CTK.Submitter (URL_APPLY)
        submit += table

	self += CTK.RawHTML ('<h2>%s</h2>' %(_('ZeroMQ parameters')))
        self += CTK.Indenter (submit)

	VALS = [("%s!endpoint"%(key),   validations.is_not_empty),
        	("%s!io_threads"%(key), validations.is_number_gt_0)]

	CTK.cfg['%s!encoder' % (key)] = 'allow'
	CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, validation=VALS, method="POST")
