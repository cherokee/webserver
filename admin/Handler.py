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
import Cherokee
import validations

URL_APPLY = '/plugin/handler/apply'

NOTE_DOCUMENT_ROOT = N_('Allows to specify an alternative Document Root path.')


class PluginHandler (CTK.Plugin):
    def __init__ (self, key, **kwargs):
        CTK.Plugin.__init__ (self, key)
        self.show_document_root = kwargs.pop('show_document_root', True)
        self.key_rule = '!'.join(self.key.split('!')[:-1])

    def AddCommon (self):
        if self.show_document_root:
            table = CTK.PropsTable()
            table.Add (_('Document Root'), CTK.TextCfg('%s!document_root'%(self.key_rule), True), _(NOTE_DOCUMENT_ROOT))

            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Indenter (table)
            self += submit

            # Publish
            VALS = [("%s!document_root"%(self.key_rule), validations.is_dev_null_or_local_dir_exists)]
            CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, validation=VALS, method="POST")
