# -*- coding: utf-8 -*-
#
# Cherokee-admin's Redirection Wizard
#
# Authors:
#      Taher Shihadeh <taher@unixwars.com>
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

#
# Tested:
# 2010/04/14: Cherokee 0.99.41
#

import re
import string
import CTK
import Wizard
import validations
import Cherokee
from util import *
from configured import *

NOTE_WELCOME_H1 = N_("Welcome to the Redirection Wizard")
NOTE_WELCOME_P1 = N_("This Wizard creates a new virtual server redirecting every request to another domain host.")

NOTE_REDIR_H1   = N_("Redirection details")
NOTE_SRC_HOST   = N_("This domain name will be used as nickname for the new virtual server. More domain names can be added to the virtual server later on.")
NOTE_TRG_HOST   = N_("Domain name to which requests will be redirected.")

PREFIX    = 'tmp!wizard!redirect'
URL_APPLY = r'/wizard/vserver/redirect/apply'

CONFIG_VSRV = """
%(pre_vsrv)s!nick = %(host_src)s
%(pre_vsrv)s!document_root = /dev/null

%(pre_vsrv)s!rule!1!match = default
%(pre_vsrv)s!rule!1!handler = redir
%(pre_vsrv)s!rule!1!handler!rewrite!1!show = 1
%(pre_vsrv)s!rule!1!handler!rewrite!1!regex = /(.*)$
%(pre_vsrv)s!rule!1!handler!rewrite!1!substring = http://%(host_trg)s/$1
"""

EXTRA_WILDCARD = """
%(pre_vsrv)s!match = wildcard
%(pre_vsrv)s!match!domain!1 = %(host_src)s
"""

EXTRA_REHOST = """
%(pre_vsrv)s!match = rehost
%(pre_vsrv)s!match!regex!1 = %(host_src)s
"""


class Commit:
    def Commit_VServer (self):
        # Incoming info
        host_src = CTK.cfg.get_val ('%s!host_src'%(PREFIX))
        host_trg = CTK.cfg.get_val ('%s!host_trg'%(PREFIX))

        # Locals
        pre_vsrv = CTK.cfg.get_next_entry_prefix('vserver')

        # Add the new rules
        config = CONFIG_VSRV % (locals())

        # Analyze the source domain
        wildcard_domain = string.letters + "_-.*?"
        is_wildcard = reduce (lambda x,y: x and y, [c in wildcard_domain for c in host_src])

        if is_wildcard:
            config += EXTRA_WILDCARD % (locals())
        else:
            config += EXTRA_REHOST % (locals())

        CTK.cfg.apply_chunk (config)

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(pre_vsrv))

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()

    def __call__ (self):
        CTK.cfg_apply_post()
        return self.Commit_VServer()


class Redirection:
    def __call__ (self):
        table = CTK.PropsTable()
        table.Add (_('Origin Domain'), CTK.TextCfg ('%s!host_src'%(PREFIX), False, {'class':'required'}), _(NOTE_SRC_HOST))
        table.Add (_('Target Domain'), CTK.TextCfg ('%s!host_trg'%(PREFIX), False, {'class':'required'}), _(NOTE_TRG_HOST))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_REDIR_H1)))
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class Welcome:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('redirect', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += Wizard.CookBookBox ('cookbook_redirs')
        cont += box
        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


VALS = [
    ('%s!host_src'%(PREFIX), validations.is_not_empty),
    ('%s!host_trg'%(PREFIX), validations.is_not_empty),
    ("%s!host_src"%(PREFIX), validations.is_new_vserver_nick),
]


# Rule
CTK.publish ('^/wizard/vserver/redirect$',   Welcome)
CTK.publish ('^/wizard/vserver/redirect/2$', Redirection)
CTK.publish (r'^%s'%(URL_APPLY), Commit, method="POST", validation=VALS)
