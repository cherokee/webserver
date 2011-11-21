# Cheroke Admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#      Stefan de Konink <stefan@konink.de>
#
# Copyright (C) 2009-2010 Alvaro Lopez Ortega
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

from Rule import RulePlugin
from util import *

URL_APPLY = '/plugin/filetime/apply'

NOTE_STATPROP  = N_("The time propery of the file to match against.")
NOTE_OPERATOR  = N_("What operator should be used to match the property against the value.")
NOTE_TIMESTAMP = N_("Unix timestamp")

STATPROPS = [
    ('', _('Choose')),
    ('atime', _('Access Time')),
    ('ctime', _('Creation Time')),
    ('mtime', _('Modification Time'))
]

OPERATORS = [
    ('', _('Choose')),
    ('le', _('less equal')),
    ('ge', _('greater equal')),
    ('eq', _('equal')),
    ('lenow', _('less equal now +')),
    ('genow', _('greater equal now +'))
]

def commit():
    # POST info
    key       = CTK.post.pop ('key', None)
    vsrv_num  = CTK.post.pop ('vsrv_num', None)
    new_stat   = CTK.post.pop ('tmp!stat', None)
    new_time = CTK.post.pop ('tmp!time', None)
    new_op  = CTK.post.pop ('tmp!op', None)

    # New entry
    if new_stat:
        next_rule, next_pre = cfg_vsrv_rule_get_next ('vserver!%s'%(vsrv_num))

        CTK.cfg['%s!match'%(next_pre)]      = 'filetime'
        CTK.cfg['%s!match!stat'%(next_pre)] = new_stat
        CTK.cfg['%s!match!time'%(next_pre)] = new_time
        CTK.cfg['%s!match!op'%(next_pre)]   = new_op

        return {'ret': 'ok', 'redirect': '/vserver/%s/rule/%s' %(vsrv_num, next_rule)}

    # Modifications
    return CTK.cfg_apply_post()


class Plugin_filetime (RulePlugin):
    def __init__ (self, key, **kwargs):
        RulePlugin.__init__ (self, key)
        is_new = key.startswith('tmp')
        props  = ({},{'class': 'noauto'})[is_new]

        op_widget = CTK.ComboCfg ('%s!op'%(key), OPERATORS, props)
        op_val    = CTK.cfg.get_val ('%s!op'%(key), 'regex')

        table = CTK.PropsTable()
        table.Add (_('Property'), CTK.ComboCfg('%s!stat'%(key), STATPROPS, props), _(NOTE_STATPROP))
        table.Add (_('Operator'), op_widget, _(NOTE_OPERATOR))
        table.Add (_('Unix Timestamp'), CTK.TextCfg('%s!time'%(key), False, props), _(NOTE_TIMESTAMP))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('key', key)
        submit += CTK.Hidden ('vsrv_num', kwargs.pop('vsrv_num', ''))
        submit += table
        self += submit

        # Validation, and Public URLs
        CTK.publish (URL_APPLY, commit, method="POST")

    def GetName (self):
        stat = CTK.cfg.get_val ('%s!stat' %(self.key), '')
        time  = CTK.cfg.get_val ('%s!time' %(self.key), '')
        op  = CTK.cfg.get_val ('%s!op' %(self.key), '')

        return "%s %s %s %s" % (_("File"), stat, op, time)
