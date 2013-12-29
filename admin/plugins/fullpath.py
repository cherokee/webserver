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

URL_APPLY = '/plugin/fullpath/apply'

NOTE_FULLPATH = N_("Full path request to which content the configuration will be applied. (Eg: /favicon.ico)")


def commit():
    # POST info
    key      = CTK.post.pop ('key', None)
    vsrv_num = CTK.post.pop ('vsrv_num', None)
    new_path = CTK.post.pop ('tmp!fullpath', None)

    # New entry
    if new_path:
        next_rule, next_pre = cfg_vsrv_rule_get_next ('vserver!%s'%(vsrv_num))
        next_path = CTK.cfg.get_next_entry_prefix ('%s!match!fullpath'%(next_pre))

        CTK.cfg['%s!match'%(next_pre)] = 'fullpath'
        CTK.cfg[next_path]             = new_path

        return {'ret': 'ok', 'redirect': '/vserver/%s/rule/%s' %(vsrv_num, next_rule)}

    # Modifications
    return CTK.cfg_apply_post()


class Plugin_fullpath (RulePlugin):
    def __init__ (self, key, **kwargs):
        RulePlugin.__init__ (self, key)
        self.vsrv_num = kwargs.pop('vsrv_num', '')

        if key.startswith('tmp'):
            return self.GUI_new()

        return self.GUI_mod()

    def GUI_new (self):
        # Table
        table = CTK.PropsTable()
        table.Add (_('Full Path'), CTK.TextCfg('%s!fullpath'%(self.key), False, {'class': 'noauto'}), _(NOTE_FULLPATH))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('key', self.key)
        submit += CTK.Hidden ('vsrv_num', self.vsrv_num)
        submit += table
        self += submit

    def GUI_mod (self):
        pre = '%s!fullpath'%(self.key)
        tmp = CTK.cfg.keys (pre)
        tmp.sort(lambda x,y: cmp(int(x), int(y)))

        if tmp:
            table = CTK.Table({'id': 'rule-table-fullpath'})
            for f in tmp:
                delete = None
                if len(tmp) > 1:
                    delete = CTK.ImageStock('del')
                    delete.bind('click', CTK.JS.Ajax (URL_APPLY,
                                                      data     = {'%s!%s'%(pre, f): ''},
                                                      complete = "$('#%s').trigger('submit_success');"%(self.id)))

                table += [CTK.TextCfg('%s!%s'%(pre,f)), delete]

            submit = CTK.Submitter (URL_APPLY)
            submit += table

            self += CTK.RawHTML ('<h3>%s</h3>' % (_("Full Web Paths")))
            self += CTK.Indenter (submit)

        # Add new
        next_bind = CTK.cfg.get_next_entry_prefix ('%s!fullpath'%(self.key))

        table = CTK.PropsTable()
        table.Add (_('Full Path'), CTK.TextCfg (next_bind, False), _(NOTE_FULLPATH))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML ('<h3>%s</h3>' % (_("Add another path")))
        self += CTK.Indenter (submit)


    def GetName (self):
        tmp = []
        for k in CTK.cfg.keys('%s!fullpath'%(self.key)):
            d = CTK.cfg.get_val ('%s!fullpath!%s'%(self.key, k))
            tmp.append(d)

        if len(tmp) == 1:
            return "%s %s" %(_("Path"), tmp[0])
        elif len(tmp) > 1:
            txt = "%s "%(_("Paths"))
            txt += ', '.join(tmp[:-1])
            txt += " or %s" %(tmp[-1])
            return txt
        else:
            return _("Path")

CTK.publish ("^%s"%(URL_APPLY), commit, method="POST")
