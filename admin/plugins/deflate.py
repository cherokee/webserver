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
import consts

URL_APPLY = '/plugin/deflate/apply'
HELPS     = [('modules_encoders_deflate', _("Deflate encoder"))]

NOTE_LEVEL = N_("Compression Level from 0 to 9, where 0 is no compression, 1 best speed, and 9 best compression.")

class Plugin_deflate (CTK.Plugin):
    def __init__ (self, key, **kwargs):
        CTK.Plugin.__init__ (self, key, **kwargs)

        table = CTK.PropsTable()
        table.Add (_("Compression Level"), CTK.ComboCfg('%s!compression_level'%(key), consts.COMPRESSION_LEVELS), _(NOTE_LEVEL))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += submit

CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, method="POST")
