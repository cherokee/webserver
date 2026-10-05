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
from CTK.i18n import _, N_
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
        next_rule, next_pre = cfg_vsrv_rule_get_next (f'vserver!{vsrv_num}')
        next_path = CTK.cfg.get_next_entry_prefix (f'{next_pre}!match!fullpath')

        CTK.cfg[f'{next_pre}!match'] = 'fullpath'
        CTK.cfg[next_path]             = new_path

        return {'ret': 'ok', 'redirect': f'/vserver/{vsrv_num}/rule/{next_rule}'}

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
        table.Add (_('Full Path'), CTK.TextCfg(f'{self.key}!fullpath', False, {'class': 'noauto'}), _(NOTE_FULLPATH))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('key', self.key)
        submit += CTK.Hidden ('vsrv_num', self.vsrv_num)
        submit += table
        self += submit

    def GUI_mod (self):
        pre = f'{self.key}!fullpath'
        tmp = CTK.cfg.keys (pre)
        tmp.sort(key=int)

        if tmp:
            table = CTK.Table({'id': 'rule-table-fullpath'})
            for f in tmp:
                delete = None
                if len(tmp) > 1:
                    delete = CTK.ImageStock('del')
                    delete.bind('click', CTK.JS.Ajax (URL_APPLY,
                                                      data     = {f'{pre}!{f}': ''},
                                                      complete = f"$('#{self.id}').trigger('submit_success');"))

                table += [CTK.TextCfg(f'{pre}!{f}'), delete]

            submit = CTK.Submitter (URL_APPLY)
            submit += table

            self += CTK.RawHTML (f"<h3>{_('Full Web Paths')}</h3>")
            self += CTK.Indenter (submit)

        # Add new
        next_bind = CTK.cfg.get_next_entry_prefix (f'{self.key}!fullpath')

        table = CTK.PropsTable()
        table.Add (_('Full Path'), CTK.TextCfg (next_bind, False), _(NOTE_FULLPATH))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML (f"<h3>{_('Add another path')}</h3>")
        self += CTK.Indenter (submit)


    def GetName (self):
        tmp = []
        for k in CTK.cfg.keys(f'{self.key}!fullpath'):
            d = CTK.cfg.get_val (f'{self.key}!fullpath!{k}')
            tmp.append(d)

        if len(tmp) == 1:
            return f"{_('Path')} {tmp[0]}"
        elif len(tmp) > 1:
            txt = f"{_('Paths')} "
            txt += ', '.join(tmp[:-1])
            txt += f" or {tmp[-1]}"
            return txt
        else:
            return _("Path")

CTK.publish (f"^{URL_APPLY}", commit, method="POST")
