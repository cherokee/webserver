# Cheroke Admin
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
import validations

from Rule import RulePlugin
from util import *

URL_APPLY = '/plugin/extensions/apply'

NOTE_EXTENSIONS  = N_("Comma-separated list of File Extension to which the configuration will be applied.")
NOTE_CHECK_LOCAL = N_("Ensure that the matched extension if part of a local file name.")

def commit():
    # POST info
    key      = CTK.post.pop ('key', None)
    vsrv_num = CTK.post.pop ('vsrv_num', None)
    new_ext  = CTK.post.pop ('tmp!extensions', None)

    # New entry
    if new_ext:
        next_rule, next_pre = cfg_vsrv_rule_get_next ('vserver!%s'%(vsrv_num))

        # Apply
        CTK.cfg['%s!match'%(next_pre)]                  = 'extensions'
        CTK.cfg['%s!match!extensions'%(next_pre)]       = new_ext
        CTK.cfg['%s!match!check_local_file'%(next_pre)] = CTK.post.pop ('tmp!check_local_file')

        return {'ret': 'ok', 'redirect': '/vserver/%s/rule/%s' %(vsrv_num, next_rule)}

    # Modifications
    return CTK.cfg_apply_post()


class Plugin_extensions (RulePlugin):
    def __init__ (self, key, **kwargs):
        RulePlugin.__init__ (self, key)
        props = ({},{'class': 'noauto'})[key.startswith('tmp')]

        table = CTK.PropsTable()
        table.Add (_('Extensions'),       CTK.TextCfg('%s!extensions'%(key), False, props), _(NOTE_EXTENSIONS))
        table.Add (_('Check local file'), CTK.CheckCfgText('%s!check_local_file'%(key), False, _('Check'), props), _(NOTE_CHECK_LOCAL))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('key', key)
        submit += CTK.Hidden ('vsrv_num', kwargs.pop('vsrv_num', ''))
        submit += table
        self += submit

        # Validation, and Public URLs
        VALS = [("tmp!extensions",       validations.is_extension_list),
                ("%s!extensions"%(key),  validations.is_extension_list)]

        CTK.publish ("^%s"%(URL_APPLY), commit, validation=VALS, method="POST")

    def GetName (self):
        tmp = CTK.cfg.get_val ('%s!extensions' %(self.key), '')
        extensions = ', '.join (tmp.split(','))
        return "Extensions %s" %(extensions)
