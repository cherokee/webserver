# Cheroke Admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#      Stefan de Konink <stefan@konink.de>
#
# Copyright (C) 2001-2013 Alvaro Lopez Ortega
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

URL_APPLY = '/plugin/fairuse/apply'

NOTE_FILENAME = N_("The filename of the GDBM file to check the source IP address against.")
NOTE_LIMIT    = N_("Set the maximum number requests per day.")
NOTE_COUNT    = N_("Should the rule itself count any request that passed true.")

def commit():
    # POST info
    key          = CTK.post.pop ('key', None)
    vsrv_num     = CTK.post.pop ('vsrv_num', None)
    new_filename = CTK.post.pop ('tmp!filename', None)
    new_limit    = CTK.post.pop ('tmp!limit', None)
    new_count    = CTK.post.pop ('tmp!count', None)

    # New entry
    if new_filename or new_limit or new_count:
        next_rule, next_pre = cfg_vsrv_rule_get_next ('vserver!%s'%(vsrv_num))

        # Apply
        CTK.cfg['%s!match'%(next_pre)]          = 'fairuse'
        CTK.cfg['%s!match!filename'%(next_pre)] = new_filename
        CTK.cfg['%s!match!limit'%(next_pre)]    = new_limit
        CTK.cfg['%s!match!count'%(next_pre)]    = new_count

        return {'ret': 'ok', 'redirect': '/vserver/%s/rule/%s' %(vsrv_num, next_rule)}

    # Modifications
    return CTK.cfg_apply_post()


class Plugin_gdbm (RulePlugin):
    def __init__ (self, key, **kwargs):
        RulePlugin.__init__ (self, key)
        props = ({},{'class': 'noauto'})[key.startswith('tmp')]

        table = CTK.PropsTable()
        table.Add (_('Filename'), CTK.TextCfg('%s!filename'%(key), False, props), _(NOTE_FILENAME))
        table.Add (_('Limit'), CTK.TextCfg('%s!limit'%(key), False, props), _(NOTE_LIMIT))
        table.Add (_('Count'), CTK.CheckCfgText('%s!count'%(key), True, ''), _(NOTE_COUNT))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('key', key)
        submit += CTK.Hidden ('vsrv_num', kwargs.pop('vsrv_num', ''))
        submit += table
        self += submit

        # Validation, and Public URLs
        VALS = [("tmp!filename",      validations.parent_is_dir),
                ("%s!filename"%(key), validations.parent_is_dir),
                ("tmp!limit",         validations.is_number_gt_0),
                ("%s!limit"%(key),    validations.is_number_gt_0)]

        CTK.publish ("^%s"%(URL_APPLY), commit, validation=VALS, method="POST")

    def GetName (self):
        return "Fair Use"
