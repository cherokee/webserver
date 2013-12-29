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

from util import *
from consts import *
from Rule import RulePlugin

URL_APPLY = '/plugin/url_arg/apply'

NOTE_ARGUMENT = N_("Argument name")
NOTE_REGEX    = N_("Regular Expression for the match")

OPTIONS = [('0', N_('Match a specific argument')),
           ('1', N_('Match any argument'))]

def commit():
    # POST info
    key       = CTK.post.pop ('key', None)
    vsrv_num  = CTK.post.pop ('vsrv_num', None)
    new_match = CTK.post.pop ('tmp!match', None)
    new_any   = CTK.post.pop ('tmp!match_any', "0")
    new_arg   = CTK.post.pop ('tmp!arg', None)

    # New entry
    if new_match:
        next_rule, next_pre = cfg_vsrv_rule_get_next ('vserver!%s'%(vsrv_num))

        CTK.cfg['%s!match'%(next_pre)]           = 'url_arg'
        CTK.cfg['%s!match!match'%(next_pre)]     = new_match
        CTK.cfg['%s!match!match_any'%(next_pre)] = new_any

        if not int(new_any):
            CTK.cfg['%s!match!arg'%(next_pre)]   = new_arg

        return {'ret': 'ok', 'redirect': '/vserver/%s/rule/%s' %(vsrv_num, next_rule)}

    # Modifications
    return CTK.cfg_apply_post()


class Plugin_url_arg (RulePlugin):
    def __init__ (self, key, **kwargs):
        RulePlugin.__init__ (self, key)
        self.vsrv_num = kwargs.pop('vsrv_num', '')

        if key.startswith('tmp'):
            return self.GUI_new()
        return self.GUI_mod()

    def GUI_new (self):
        any   = CTK.ComboCfg('%s!match_any'%(self.key), trans_options(OPTIONS), {'class': 'noauto'})
        table = CTK.PropsTable()
        table.Add (_('Match type'), any, '')
        table.Add (_('Argument'),           CTK.TextCfg('%s!arg'%(self.key), False, {'class': 'noauto'}), _(NOTE_ARGUMENT))
        table.Add (_('Regular Expression'), CTK.TextCfg('%s!match'%(self.key), False, {'class': 'noauto'}), _(NOTE_REGEX))

        # Special events
        any.bind ('change', """if ($(this).val() == 0) {
                                 $("#%(row)s").show();
                               } else {
                                 $("#%(row)s").hide(); }""" %({'row': table[1].id}))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('key', self.key)
        submit += CTK.Hidden ('vsrv_num', self.vsrv_num)
        submit += table
        self += submit

    def GUI_mod (self):
        table = CTK.PropsTable()
        table.Add (_('Match type'),  CTK.ComboCfg('%s!match_any'%(self.key), trans_options(OPTIONS)), '')
        if not int(CTK.cfg.get_val('%s!match_any'%(self.key), 0)):
            table.Add (_('Argument'),       CTK.TextCfg('%s!arg'%(self.key), False), _(NOTE_ARGUMENT))
        table.Add (_('Regular Expression'), CTK.TextCfg('%s!match'%(self.key), False), _(NOTE_REGEX))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('key', self.key)
        submit += CTK.Hidden ('vsrv_num', self.vsrv_num)
        submit += table
        self += submit

    def GetName (self):
        match = CTK.cfg.get_val ('%s!match'%(self.key), '')

        if int(CTK.cfg.get_val ('%s!match_any'%(self.key), "0")):
            return "Any arg. matches ~ %s" %(match)

        arg = CTK.cfg.get_val ('%s!arg'%(self.key), '')
        return "Arg %s matches %s" %(arg, match)


# Validation, and Public URLs
CTK.publish ("^%s"%(URL_APPLY), commit, method="POST")
