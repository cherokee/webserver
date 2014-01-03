# -*- coding: utf-8 -*-
#
# Cherokee-admin's Hot Linking Prevention Wizard
#
# Authors:
#      Taher Shihadeh <taher@unixwars.com>
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
import CTK
import Wizard
import validations
import Cherokee

from util import *
from configured import *
from consts import *

NOTE_WELCOME_H1 = N_("Welcome to the Hotlinking Wizard")
NOTE_WELCOME_P1 = N_("This wizard stops other domains from hot-linking your media files.")
NOTE_RULE_H1    = N_("Anti-HotLinking Information")
NOTE_DOMAIN     = N_("Domain allowed to access the files. Eg: example.com")
NOTE_REDIR      = N_("Path to the public resource to use. Eg: /images/forbidden.jpg")
NOTE_TYPE       = N_("How to handle hot-linking requests.")

PREFIX     = 'tmp!wizard!hotlinking'
EXTENSIONS = ['jpg', 'jpeg', 'gif', 'png', 'flv']

URL_APPLY         = r'/wizard/vserver/hotlinking/apply'
URL_APPLY_REFRESH = r'/wizard/vserver/hotlinking/apply/refresh'

CONFIG_RULES = """
%(rule_pre)s!match = and
%(rule_pre)s!match!left = extensions
%(rule_pre)s!match!left!extensions = %(extensions)s
%(rule_pre)s!match!right = and
%(rule_pre)s!match!right!left = header
%(rule_pre)s!match!right!left!header = Referer
%(rule_pre)s!match!right!left!type = provided
%(rule_pre)s!match!right!right = not
%(rule_pre)s!match!right!right!right = header
%(rule_pre)s!match!right!right!right!header = Referer
%(rule_pre)s!match!right!right!right!match = ^https?://(.*?\.)?%(domain)s
%(rule_pre)s!match!right!right!right!type = regex
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
        extensions  = ','.join(EXTENSIONS)

        # Overwrite a previous rule or create a new one
        tmp = re.findall ("vserver!%s!rule!(\d+)!match!left!extensions = %s" %(vsrv_num, extensions), CTK.cfg.serialize())
        if tmp:
            vsrv_pre = 'vserver!%s' %(vsrv_num)
            rule_pre = 'vserver!%s!rule!%s' %(vsrv_num, tmp[0])
            prio     = int(tmp[0])
        else:
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


class RuleData:
    class Refresh (CTK.Container):
        def __init__ (self):
            CTK.Container.__init__ (self)

            tipe = CTK.cfg.get_val ('%s!type'%(PREFIX))
            if tipe == 'redir':
                table = CTK.PropsTable()
                table.Add (_('Redirection'), CTK.TextCfg('%s!redirection'%(PREFIX), False, {}), _(NOTE_REDIR))

                submit = CTK.Submitter (URL_APPLY_REFRESH)
                submit += table

                self += submit

    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_RULE_H1)))

        vsrv_num = CTK.cfg.get_val("%s!vsrv_num"%(PREFIX))
        nick = CTK.cfg.get_val("vserver!%s!nick"%(vsrv_num))
        if not '.' in nick:
            nick = "example.com"

        refresh = CTK.Refreshable ({'id':'hotlinking_wizard_refresh'})
        refresh.register (lambda: self.Refresh().Render())

        combo_widget = CTK.ComboCfg ('%s!type'%(PREFIX), trans_options(TYPES))
        combo_widget.bind('change', refresh.JS_to_refresh())

        table = CTK.PropsTable()
        table.Add (_('Domain Name'), CTK.TextCfg ('%s!domain'%(PREFIX), False, {'value': nick}), _(NOTE_DOMAIN))
        table.Add (_('Reply type'), combo_widget, _(NOTE_TYPE))

        submit = CTK.Submitter (URL_APPLY_REFRESH)
        submit += table
        submit += refresh
        cont += submit

        # Global Submit
        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')
        cont += submit

        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
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
CTK.publish (r'^%s'%(URL_APPLY),         Commit,             method="POST", validation=VALS)
CTK.publish (r'^%s'%(URL_APPLY_REFRESH), CTK.cfg_apply_post, method="POST", validation=VALS)
