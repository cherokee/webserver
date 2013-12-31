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

URL_APPLY = '/plugin/from/apply'

NOTE_FROM = N_("IP or Subnet to check the connection origin against.")

def commit():
    # POST info
    key      = CTK.post.pop ('key', None)
    vsrv_num = CTK.post.pop ('vsrv_num', None)
    new_from = CTK.post.pop ('tmp!from', None)

    # New entry
    if new_from:
        next_rule, next_pre = cfg_vsrv_rule_get_next ('vserver!%s'%(vsrv_num))
        next_from = CTK.cfg.get_next_entry_prefix ('%s!match!from'%(next_pre))

        CTK.cfg['%s!match'%(next_pre)] = 'from'
        CTK.cfg[next_from]             = new_from

        return {'ret': 'ok', 'redirect': '/vserver/%s/rule/%s' %(vsrv_num, next_rule)}

    # Modifications
    return CTK.cfg_apply_post()


class Plugin_from (RulePlugin):
    def __init__ (self, key, **kwargs):
        RulePlugin.__init__ (self, key)
        self.vsrv_num = kwargs.pop('vsrv_num', '')

        # Validation, and Public URLs
        VAL = [('^%s!from$'%(key),   validations.is_ip_or_netmask),
               ('^%s!from!.+'%(key), validations.is_ip_or_netmask)]

        CTK.publish ("^%s"%(URL_APPLY), commit, validation=VAL, method="POST")

        if key.startswith('tmp'):
            return self.GUI_new()

        return self.GUI_mod()

    def GUI_new (self):
        # Table
        table = CTK.PropsTable()
        table.Add (_('Connected from'), CTK.TextCfg('%s!from'%(self.key), False, {'class': 'noauto'}), _(NOTE_FROM))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('key', self.key)
        submit += CTK.Hidden ('vsrv_num', self.vsrv_num)
        submit += table
        self += submit

    def GUI_mod (self):
        pre = '%s!from'%(self.key)
        tmp = CTK.cfg.keys (pre)
        tmp.sort(lambda x,y: cmp(int(x), int(y)))

        if tmp:
            table = CTK.Table({'id': 'rule-table-from'})
            for f in tmp:
                delete = None
                if len(tmp) > 1:
                    delete = CTK.ImageStock('del')
                    delete.bind('click', CTK.JS.Ajax (URL_APPLY,
                                                      data     = {'%s!%s'%(pre, f): ''},
                                                      complete = "$('#%s').trigger('submit_success');"%(self.id)))

                table += [CTK.TextCfg('%s!%s'%(pre,f)), delete]

            self += CTK.RawHTML ('<h3>%s</h3>' % (_("IPs and Subnets")))
            self += CTK.Indenter (table)

        # Add new
        next_from = CTK.cfg.get_next_entry_prefix ('%s!from'%(self.key))

        table = CTK.PropsTable()
        table.Add (_('New IP or Subnet'), CTK.TextCfg(next_from, False), _(NOTE_FROM))
        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML ('<h3>%s</h3>' % (_("Add another entry")))
        self += CTK.Indenter (submit)


    def GetName (self):
        tmp = []
        for k in CTK.cfg.keys('%s!from'%(self.key)):
            d = CTK.cfg.get_val ('%s!from!%s'%(self.key, k))
            tmp.append(d)

        txt = '%s ' %(_("From"))
        if len(tmp) > 1:
            txt += ', '.join(tmp[:-1])
            txt += " or %s" %(tmp[-1])
        elif len(tmp) == 1:
            txt += tmp[0]
        return txt
