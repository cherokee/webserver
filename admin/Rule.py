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

import re
import copy
import CTK

from util import *
from consts import *


DEFAULT_RULE_WARNING = N_('The default match ought not to be changed.')

URL_APPLY_LOGIC = "/rule/logic/apply"


class RulePlugin (CTK.Plugin):
    def __init__ (self, key):
        CTK.Plugin.__init__ (self, key)

    def GetName (self):
        raise NotImplementedError

def RuleButtons_apply():
    def move (old, new):
        val = CTK.cfg.get_val(old)
        tmp = copy.deepcopy (CTK.cfg[old])
        del (CTK.cfg[old])

        CTK.cfg[new] = val
        CTK.cfg.set_sub_node (new, tmp)

    # Data collection
    key = CTK.post['key']
    op  = CTK.post['op']

    # Apply
    if op == 'NOT':
        if CTK.cfg.get_val(key) == 'not':
            move ("%s!right"%(key), key)
        else:
            move (key, "%s!right"%(key))
            CTK.cfg[key] = 'not'

    elif op == 'AND':
        move (key, "%s!left"%(key))
        CTK.cfg[key] = 'and'

    elif op == 'OR':
        move (key, "%s!left"%(key))
        CTK.cfg[key] = 'or'

    elif op == 'DEL':
        parent_key  = '!'.join (key.split('!')[:-1])
        parent_rule = CTK.cfg.get_val(parent_key)

        if parent_rule == 'not':
            move ("%s!right"%(parent_key), parent_key)
        elif parent_rule in ('and', 'or'):
            if key.endswith ('right'):
                move ("%s!left"%(parent_key), parent_key)
            else:
                move ("%s!right"%(parent_key), parent_key)
        else:
            del(CTK.cfg[key])

    return CTK.cfg_reply_ajax_ok()


class RuleButtons (CTK.Box):
    def __init__ (self, key, depth):
        CTK.Box.__init__ (self, {'class': 'rulebuttons'})

        if depth == 0:
            opts = ['OR', 'AND', 'NOT']
        else:
            opts = ['OR', 'AND', 'NOT', 'DEL']

        for op in opts:
            submit = CTK.Submitter (URL_APPLY_LOGIC)
            submit += CTK.Hidden ('key', key)
            submit += CTK.Hidden ('op', op)
            submit += CTK.SubmitterButton (op)
            submit.bind ('submit_success', "$(this).trigger('changed');")
            self += submit


class Rule (CTK.Box):
    def __init__ (self, key, depth=0):
        assert type(key) == str, type(key)
        assert type(depth) == int, type(depth)

        CTK.Box.__init__ (self, {'class': 'rule'})
        self.key   = key
        self.depth = depth

    def Render (self):
        # Default Rule
        value = CTK.cfg.get_val (self.key)
        if value == 'default':
            self += CTK.Notice ('important-information', CTK.RawHTML (_(DEFAULT_RULE_WARNING)))
            return CTK.Box.Render (self)

        # Special rule types
        elif value == "and":
            self.props['class'] += ' rule-and'
            self += Rule ('%s!left' %(self.key), self.depth+1)
            self += CTK.Box({'class':'rule-label-and'}, CTK.RawHTML(_("And")))
            self += Rule ('%s!right'%(self.key), self.depth+1)
            self += RuleButtons (self.key, self.depth)
            return CTK.Box.Render (self)

        elif value == "or":
            self.props['class'] += ' rule-or'
            self += Rule ('%s!left' %(self.key), self.depth+1)
            self += CTK.Box({'class':'rule-label-or'}, CTK.RawHTML(_("Or")))
            self += Rule ('%s!right'%(self.key), self.depth+1)
            self += RuleButtons (self.key, self.depth)
            return CTK.Box.Render (self)

        elif value == "not":
            self.props['class'] += ' rule-not'
            self += CTK.Box({'class':'rule-label-not'}, CTK.RawHTML(_("Not")))
            self += Rule ('%s!right'%(self.key), self.depth+1)
            self += RuleButtons (self.key, self.depth)
            return CTK.Box.Render (self)

        # Regular rules
        vsrv_num = self.key.split('!')[1]

        if not CTK.cfg.get_val(self.key):
            rules = [('', _('Select'))] + trans_options(RULES)
        else:
            rules = trans_options(RULES[:])

        table = CTK.PropsTable()
        modul = CTK.PluginSelector (self.key, rules, vsrv_num=vsrv_num)
        table.Add (_('Rule Type'), modul.selector_widget, '')

        self += table
        self += modul

        if CTK.cfg.get_val(self.key):
            self += RuleButtons (self.key, self.depth)

        # Render
        return CTK.Box.Render (self)

    def GetName (self):
        # Default Rule
        value = CTK.cfg.get_val (self.key)
        if value == 'default':
            return _('Default')

        # Logic ops
        elif value == "and":
            rule1 = Rule ('%s!left' %(self.key), self.depth+1)
            rule2 = Rule ('%s!right'%(self.key), self.depth+1)
            return '(%s AND %s)' %(rule1.GetName(), rule2.GetName())

        elif value == "or":
            rule1 = Rule ('%s!left' %(self.key), self.depth+1)
            rule2 = Rule ('%s!right'%(self.key), self.depth+1)
            return '(%s OR %s)' %(rule1.GetName(), rule2.GetName())

        elif value == "not":
            rule = Rule ('%s!right'%(self.key), self.depth+1)
            return '(NOT %s)' %(rule.GetName())

        # No rule (yet)
        if not value:
            return ''

        # Regular rules
        plugin = CTK.instance_plugin (value, self.key)
        name = plugin.GetName()

        # There are no further replacements within this string. Let's
        # replace the '%' by its HTML equivalent so it does not cause
        # trouble later on. For instance, if the title of a Regular
        # Expression rule contained a subsctring like '%[', it would
        # make the Python string replacement mechanism fail.
        #
        tmp = CTK.escape_html (name)
        return tmp.replace ('%', '&#37;')


CTK.publish (r"^%s"%(URL_APPLY_LOGIC), RuleButtons_apply, method="POST")
