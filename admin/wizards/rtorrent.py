# -*- coding: utf-8 -*-
#
# rTorrent Wizard
#
# Authors:
#      Taher Shihadeh <taher@unixwars.com>
#      Antonio Perez  <aperez@skarcha.com>
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

import os
import re
import CTK
import Wizard
import validations
from util import *

NOTE_WELCOME_H1 = N_("Welcome to the rTorrent Wizard")
NOTE_WELCOME_P1 = N_("""<a target="_blank" href="http://libtorrent.rakshasa.no/">rTorrent</a> is a text-based ncurses BitTorrent client written in C++, based on the libTorrent libraries for Unix (not to be confused with libtorrent by Arvid Norberg), whose author's goal is “a focus on high performance and good code”""")

NOTE_LOCAL_H1   = N_("Application Source Code")
NOTE_CONNECTION = N_("Where rTorrent XMLRPC is available. The host:port pair, or the Unix socket path.")
NOTE_WEB_DIR    = N_("Web directory where you want rTorrent XMLRPC to be accessible. (Example: /RPC2)")

PREFIX          = 'tmp!wizard!rtorrent'
URL_APPLY       = r'/wizard/vserver/rtorrent/apply'

CONFIG_DIR = """
%(rule_pre_1)s!match = request
%(rule_pre_1)s!match!request = %(web_dir)s
%(rule_pre_1)s!handler = scgi
%(rule_pre_1)s!handler!balancer = round_robin
%(rule_pre_1)s!handler!balancer!source!1 = %(src_num)d
"""

CONFIG_SOURCE = """
%(source)s!nick = rTorrent XMLRPC
%(source)s!type = host
%(source)s!host = %(connection)s
"""

class Commit:
    def Commit_Rule (self):
        vsrv_num = CTK.cfg.get_val ('%s!vsrv_num'%(PREFIX))
        pre_vsrv = 'vserver!%s' %(vsrv_num)

        # rtorrent
        web_dir    = CTK.cfg.get_val('%s!web_dir'    %(PREFIX))
        connection = CTK.cfg.get_val('%s!connection' %(PREFIX))

        config = CONFIG_DIR

        # Add source
        source = cfg_source_find_interpreter (None, 'rTorrent XMLRPC')
        if not source:
            x, source = cfg_source_get_next ()
            config += CONFIG_SOURCE

        src_num = int(source.split('!')[-1])

        rule_n, x = cfg_vsrv_rule_get_next (pre_vsrv)
        rule_pre_1 = '%s!rule!%d' % (pre_vsrv, rule_n)

        config = config %(locals())
        CTK.cfg.apply_chunk (config)

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(pre_vsrv))

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()

    def __call__ (self):
        if CTK.post.pop('final'):
            CTK.cfg_apply_post()
            return self.Commit_Rule()

        return CTK.cfg_apply_post()


class LocalSource:
    def __call__ (self):
        table = CTK.PropsTable()
        table.Add (_('Web Directory'), CTK.TextCfg ('%s!web_dir'%(PREFIX), False, {'value': '/RPC2', 'class': 'noauto'}), _(NOTE_WEB_DIR))
        table.Add (_('Connection'), CTK.TextCfg ('%s!connection'%(PREFIX), False, {'value': 'localhost:5000', 'class': 'noauto'}), _(NOTE_CONNECTION))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_LOCAL_H1)))
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class Welcome:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('rtorrent', {'class': 'wizard-descr'})
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


def is_valid_webdir (web_dir):
    web_dir = validations.is_dir_formatted (web_dir)
    vsrv_num = CTK.cfg.get_val ('%s!vsrv_num'%(PREFIX))
    vsrv_pre = 'vserver!%s' %(vsrv_num)

    rule = cfg_vsrv_rule_find_regexp (vsrv_pre, '^'+web_dir)
    if rule:
        raise ValueError, _("Already configured: %s"%(web_dir))

    return web_dir

VALS = [
    ('%s!web_dir' %(PREFIX), validations.is_not_empty),
    ('%s!web_dir' %(PREFIX), is_valid_webdir),
]

# Rule
CTK.publish ('^/wizard/vserver/(\d+)/rtorrent$',   Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/rtorrent/2$', LocalSource)

# Common
CTK.publish (r'^%s'%(URL_APPLY), Commit, method="POST", validation=VALS)
