# -*- coding: utf-8 -*-
#
# Cherokee-admin's Hot Linking Prevention Wizard
#
# Authors:
#      Taher Shihadeh <taher@unixwars.com>
#
# Copyright (C) 2010 Alvaro Lopez Ortega
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

#
# Tested:
# 2010/04/14: Cherokee 0.99.41
#

import re
import CTK
import Wizard
import validations
import Cherokee
from util import *
from configured import *

NOTE_WELCOME_H1 = N_("Welcome to the Hotlinking Wizard")
NOTE_WELCOME_P1 = N_("This wizard stops other domains from hot-linking your media files.")
NOTE_RULE_H1    = N_("Anti-HotLinking Information")
NOTE_DOMAIN     = N_("Domain allowed to access the files. Eg: example.com")
NOTE_REDIR      = N_("Path to the public resource to use. Eg: /images/forbidden.jpg")
NOTE_TYPE       = N_("How to handle hot-linking requests.")
NOTE_CONFIRM_H1 = N_("Recollection completed")
NOTE_CONFIRM_P1 = N_("The required Information has been collected. Proceed to create the necessary rules to prevent hot-linking from happening.")

PREFIX    = 'tmp!wizard!hotlinking'
URL_APPLY = r'/wizard/vserver/hotlinking/apply'

CONFIG_RULES = """
%(rule_pre)s!match = and
%(rule_pre)s!match!left = extensions
%(rule_pre)s!match!left!extensions = jpg,jpeg,gif,png,flv
%(rule_pre)s!match!right = not
%(rule_pre)s!match!right!right = header
%(rule_pre)s!match!right!right!header = Referer
%(rule_pre)s!match!right!right!match = ^($|https?://(.*?\.)?%(domain)s)
"""

REPLY_FORBIDDEN = """
%(rule_pre)s!handler = custom_error
%(rule_pre)s!handler!error = 403
"""

REPLY_REDIR = """
%(rule_pre)s!handler = redir
%(rule_pre)s!handler!rewrite!1!show = 0
%(rule_pre)s!handler!rewrite!1!substring = %(redirection)s
"""

REPLY_EMPTY = """
%(rule_pre)s!handler = empty_gif
"""

CONFIG_EXCEPTION = """
%(pre_rule_plus1)s!handler = file
%(pre_rule_plus1)s!match = fullpath
%(pre_rule_plus1)s!match!fullpath!1 = %(redirection)s
"""

TYPES = [
    ('error', N_('Show error')),
    ('redir', N_('Redirect')),
    ('empty', N_('1x1 Transparent GIF'))
]

class Commit:
    def Commit_Rule (self):
        vsrv_num    = CTK.cfg.get_val ('%s!vsrv_num'%(PREFIX))
        tipe        = CTK.cfg.get_val ('%s!type'%(PREFIX))
        redirection = CTK.cfg.get_val ('%s!redirection'%(PREFIX))
        domain      = CTK.cfg.get_val ('%s!domain'%(PREFIX))
        domain      = domain.replace ('.', "\\.")

        vsrv_pre = 'vserver!%s' %(vsrv_num)
        prio, rule_pre = cfg_vsrv_rule_get_next (vsrv_pre)

        # Add the new rules
        if tipe == 'redir' and redirection:
            config = (CONFIG_RULES + REPLY_REDIR) % (locals())
            for ext in ['jpg','jpeg','gif','png','flv']:
                if redirection.endswith('.%s' % ext):
                    tmp = prio + 100
                    pre_rule_plus1 = '%s!%d' % ('!'.join(rule_pre.split('!')[:-1]), tmp)
                    config += CONFIG_EXCEPTION % (locals())
                    break

        elif tipe == 'empty':
            config = (CONFIG_RULES + REPLY_EMPTY) % (locals())
        else:
            config = (CONFIG_RULES + REPLY_FORBIDDEN) % (locals())

        CTK.cfg.apply_chunk (config)

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(vsrv_pre))

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()

    def __call__ (self):
        if CTK.post.pop('final'):
            # Apply POST
            CTK.cfg_apply_post()
            return self.Commit_Rule()

        return CTK.cfg_apply_post()

class Confirm:
    def __call__ (self):
        redir  = CTK.cfg.get_val ('%s!redirection'%(PREFIX))
        domain = CTK.cfg.get_val ('%s!domain'%(PREFIX))
        tipe   = CTK.cfg.get_val ('%s!type'%(PREFIX))
        for entry in TYPES:
            desc = entry[1]
            if entry[0] == tipe: break

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')

        table = CTK.Table({'id':'wizard-hotlinking'})
        table += [CTK.RawHTML(x) for x in ['<b>%s</b> '%(_('Domain')), domain]]
        table += [CTK.RawHTML(x) for x in ['<b>%s</b> '%(_('Type')),  _(desc)]]
        if tipe == 'redir' and redir:
            table += [CTK.RawHTML(x) for x in ['<b>%s</b> '%(_('Redirection')),  redir]]

        cont  = CTK.Container()
        cont  += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_CONFIRM_H1)))
        cont  += CTK.RawHTML (_(NOTE_CONFIRM_P1))
        cont  += submit
        cont  += table
        cont  += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class RuleData:
    class Refresh (CTK.Container):
        def __init__ (self):
            CTK.Container.__init__ (self)

            tipe = CTK.cfg.get_val ('%s!type'%(PREFIX))
            if tipe == 'redir':
                table = CTK.PropsTable()
                table.Add (_('Redirection'), CTK.TextCfg('%s!redirection'%(PREFIX), False, {}), _(NOTE_REDIR))
                self += table

    def __call__ (self):
        vsrv_num = CTK.cfg.get_val("%s!vsrv_num"%(PREFIX))
        nick = CTK.cfg.get_val("vserver!%s!nick"%(vsrv_num))
        if not '.' in nick:
            nick = "example.com"

        refresh = CTK.Refreshable ({'id':'hotlinking_wizard_refresh'})
        refresh.register (lambda: self.Refresh().Render())
        combo_widget = CTK.ComboCfg ('%s!type'%(PREFIX), TYPES)
        combo_widget.bind('change', refresh.JS_to_refresh())

        table = CTK.PropsTable()
        table.Add (_('Domain Name'), CTK.TextCfg ('%s!domain'%(PREFIX), False, {'value': nick}), _(NOTE_DOMAIN))
        table.Add (_('Reply type'), combo_widget, _(NOTE_TYPE))

        submit = CTK.Submitter (URL_APPLY)
        submit += table
        submit += refresh

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_RULE_H1)))
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevNext_Auto()
        return cont.Render().toStr()


class Welcome:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('hotlinking', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        cont += box

        # Send the VServer num if it's a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


VALS = [
    ("%s!redirection", validations.is_not_empty),
    ("%s!redirection", validations.is_path),
]


# Rule
CTK.publish ('^/wizard/vserver/(\d+)/hotlinking$',   Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/hotlinking/2$', RuleData)
CTK.publish ('^/wizard/vserver/(\d+)/hotlinking/3$', Confirm)
CTK.publish (r'^%s$'%(URL_APPLY), Commit, method="POST", validation=VALS)
