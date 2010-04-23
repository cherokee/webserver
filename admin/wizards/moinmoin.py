# -*- coding: utf-8 -*-
#
# Cherokee-admin's MoinMoin Wizard
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
# 2009/12/xx: MoinMoin 1.8.5 / Cherokee 0.99.32b
# 2010/04/15: MoinMoin 1.8.5 / Cherokee 0.99.41
#

import re
import CTK
import Wizard
import validations
from util import *

NOTE_WELCOME_H1   = N_("Welcome to the MoinMoin Wizard")
NOTE_WELCOME_P1   = N_('<a target="_blank" href="http://moinmo.in/">MoinMoin</a>  is an advanced, easy to use and extensible WikiEngine with a large community of users.')
NOTE_WELCOME_P2   = N_(' Said in a few words, it is about collaboration on easily editable web pages.')

NOTE_COMMON_H1    = N_("MoinMoin Wiki Instance")
NOTE_COMMON_WIKI  = N_("Local path to the MoinMoin Wiki Instance.")
NOTE_COMMON_SPAWN = N_("Complete path to the FastCGI spawner, normally spawn-fcgi.")

NOTE_HOST         = N_("Host name of the virtual server that is about to be created.")
NOTE_HOST_H1      = N_("New Virtual Server Details")

NOTE_WEBDIR       = N_("Public web directory to access the project.")
NOTE_WEBDIR_H1    = N_("Public Web Direcoty")

ERROR_NO_MOINMOIN_WIKI = N_("It does not look like a MoinMoin Wiki Instance.")
ERROR_NO_MOINMOIN = N_("MoinMoin doesn't seem to be correctly installed.")

PREFIX          = 'tmp!wizard!moinmoin'
URL_APPLY       = r'/wizard/vserver/moinmoin/apply'

# TCP port value is automatically asigned to one currently not in use
SOURCE = """
source!%(src_num)d!env_inherited = 1
source!%(src_num)d!host = 127.0.0.1:%(src_port)d
source!%(src_num)d!interpreter = %(spawn_fcgi)s -n -a 127.0.0.1 -p %(src_port)d -- %(moinmoin_wiki)s/server/moin.fcg
source!%(src_num)d!nick = MoinMoin %(src_num)d
source!%(src_num)d!type = interpreter
"""

CONFIG_VSERVER = SOURCE + """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = /dev/null

%(vsrv_pre)s!rule!200!match = directory
%(vsrv_pre)s!rule!200!match!directory = %(moinmoin_static)s
%(vsrv_pre)s!rule!200!match!final = 1
%(vsrv_pre)s!rule!200!document_root = %(moinmoin_wiki)s/htdocs
%(vsrv_pre)s!rule!200!encoder!deflate = 0
%(vsrv_pre)s!rule!200!encoder!gzip = 0
%(vsrv_pre)s!rule!200!handler = file
%(vsrv_pre)s!rule!200!handler!iocache = 1
%(vsrv_pre)s!rule!200!no_log = 0
%(vsrv_pre)s!rule!200!only_secure = 0

%(vsrv_pre)s!rule!100!match = default
%(vsrv_pre)s!rule!100!match!final = 1
%(vsrv_pre)s!rule!100!handler = fcgi
%(vsrv_pre)s!rule!100!handler!balancer = round_robin
%(vsrv_pre)s!rule!100!handler!balancer!source!1 = %(src_num)d
%(vsrv_pre)s!rule!100!handler!change_user = 0
%(vsrv_pre)s!rule!100!handler!check_file = 0
%(vsrv_pre)s!rule!100!handler!error_handler = 1
%(vsrv_pre)s!rule!100!handler!pass_req_headers = 1
%(vsrv_pre)s!rule!100!handler!x_real_ip_access_all = 0
%(vsrv_pre)s!rule!100!handler!x_real_ip_enabled = 0
%(vsrv_pre)s!rule!100!handler!xsendfile = 0
"""

CONFIG_DIR = SOURCE + """
%(rule_pre_2)s!match = directory
%(rule_pre_2)s!document_root = %(moinmoin_wiki)s/htdocs
%(rule_pre_2)s!match!directory = %(web_dir)s%(moinmoin_static)s
%(rule_pre_2)s!match!final = 1
%(rule_pre_2)s!handler = file
%(rule_pre_2)s!handler!iocache = 1

%(rule_pre_1)s!match = directory
%(rule_pre_1)s!match!directory = %(web_dir)s
%(rule_pre_1)s!match!final = 1
%(rule_pre_1)s!handler = fcgi
%(rule_pre_1)s!handler!balancer = round_robin
%(rule_pre_1)s!handler!balancer!source!1 = %(src_num)d
%(rule_pre_1)s!handler!change_user = 0
%(rule_pre_1)s!handler!check_file = 0
%(rule_pre_1)s!handler!error_handler = 1
%(rule_pre_1)s!handler!pass_req_headers = 1
"""

SRC_PATHS = [
    "/usr/share/pyshared/moinmoin",
    "/usr/share/pyshared/MoinMoin",
    "/usr/share/moinmoin",
    "/usr/share/moin",
    "/opt/local/share/moin"
]

DEFAULT_BINS    = ['spawn-fcgi']

class Commit:
    def Commit_VServer (self):
        # Incoming info
        new_host = CTK.cfg.get_val('%s!new_host'%(PREFIX))
        moinmoin_wiki   = CTK.cfg.get_val('%s!moinmoin_wiki'%(PREFIX))
        moinmoin_static = find_url_prefix_static (moinmoin_wiki)
        spawn_fcgi      = CTK.cfg.get_val('%s!spawn_fcgi'%(PREFIX))

        # Create the new Virtual Server
        vsrv_pre = CTK.cfg.get_next_entry_prefix('vserver')
        CTK.cfg['%s!nick'%(vsrv_pre)] = new_host
        Wizard.CloneLogsCfg_Apply ('%s!logs_as_vsrv'%(PREFIX), vsrv_pre)

        # Locals
        src_num, src_pre = cfg_source_get_next ()
        src_port = cfg_source_find_free_port ()

        # Add the new rules
        config = CONFIG_VSERVER %(locals())
        CTK.cfg.apply_chunk (config)

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(vsrv_pre))
        CTK.cfg.normalize ('vserver')

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()


    def Commit_Rule (self):
        vsrv_num = CTK.cfg.get_val ('%s!vsrv_num'%(PREFIX))
        vsrv_pre = 'vserver!%s' %(vsrv_num)

        # Incoming info
        web_dir         = CTK.cfg.get_val('%s!web_dir'%(PREFIX))
        moinmoin_wiki   = CTK.cfg.get_val('%s!moinmoin_wiki'%(PREFIX))
        moinmoin_static = find_url_prefix_static (moinmoin_wiki)
        spawn_fcgi      = CTK.cfg.get_val('%s!spawn_fcgi'%(PREFIX))

        # locals
        rule_n, x        = cfg_vsrv_rule_get_next (vsrv_pre)
        rule_pre_1       = '%s!rule!%d'%(vsrv_pre, rule_n+1)
        rule_pre_2       = '%s!rule!%d'%(vsrv_pre, rule_n+2)
        src_num, src_pre = cfg_source_get_next ()
        src_port         = cfg_source_find_free_port ()

        # Add the new rules
        config = CONFIG_DIR %(locals())
        CTK.cfg.apply_chunk (config)

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(vsrv_pre))

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()


    def __call__ (self):
        if CTK.post.pop('final'):
            # Apply POST
            CTK.cfg_apply_post()

            # VServer or Rule?
            if CTK.cfg.get_val ('%s!vsrv_num'%(PREFIX)):
                return self.Commit_Rule()
            return self.Commit_VServer()

        return CTK.cfg_apply_post()


class WebDirectory:
    def __call__ (self):
        table = CTK.PropsTable()
        table.Add (_('Web Directory'), CTK.TextCfg ('%s!web_dir'%(PREFIX), False, {'value': "/moinmoin", 'class': 'noauto'}), _(NOTE_WEBDIR))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WEBDIR_H1)))
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class Host:
    def __call__ (self):
        table = CTK.PropsTable()
        table.Add (_('New Host Name'),    CTK.TextCfg ('%s!new_host'%(PREFIX), False, {'value': 'moinmoin.example.com', 'class': 'noauto'}), _(NOTE_HOST))

        submit  = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')
        submit += table

        cont  = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_HOST_H1)))
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class Common:
    def __call__ (self):
        guessed_src = path_find_w_default (SRC_PATHS)
        spawn_fcgi  = path_find_binary (DEFAULT_BINS)

        submit = CTK.Submitter (URL_APPLY)
        table  = CTK.PropsTable()
        table.Add (_('MoinMoin Directory'), CTK.TextCfg ('%s!moinmoin_wiki'%(PREFIX), False, {'value':guessed_src}), _(NOTE_COMMON_WIKI))

        if spawn_fcgi:
            submit += CTK.Hidden('%s!spawn_fcgi'%(PREFIX), spawn_fcgi)
        else:
            table.Add (_('FastCGI spawner binary'), CTK.TextCfg ('%s!spawn_fcgi'%(PREFIX), False), _(NOTE_COMMON_SPAWN))

        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_COMMON_H1)))
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevNext_Auto()
        return cont.Render().toStr()


class Welcome:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('moinmoin', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P2)))
        cont += box

        # Send the VServer num if it is a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


def find_url_prefix_static (path):
    try:
        import MoinMoin.config
        return MoinMoin.config.url_prefix_static
    except ImportError:
        RE_MATCH = """url_prefix_static = '(.*?)'"""
        filename = os.path.join (path, "config/wikiconfig.py")
        regex = re.compile(RE_MATCH, re.DOTALL)
        fullname = get_real_path (filename)
        match = regex.search (open(fullname).read())
        if match:
            return match.groups()[0]
        raise ValueError, _(ERROR_NO_MOINMOIN)

def is_moinmoin_wiki (path):
    path = validations.is_local_dir_exists (path)
    manage = os.path.join (path, "config/wikiconfig.py")
    try:
        validations.is_local_file_exists (manage)
    except:
        raise ValueError, _(ERROR_NO_MOINMOIN_WIKI)
    return path


VALS = [
    ('%s!new_host'     %(PREFIX), validations.is_not_empty),
    ('%s!new_webdir'   %(PREFIX), validations.is_not_empty),
    ('%s!moinmoin_wiki'%(PREFIX), validations.is_not_empty),

    ("%s!new_host"     %(PREFIX), validations.is_new_vserver_nick),
    ("%s!new_webdir"   %(PREFIX), validations.is_dir_formatted),
    ("%s!moinmoin_wiki"%(PREFIX), is_moinmoin_wiki),
]

# VServer
CTK.publish ('^/wizard/vserver/moinmoin$',   Welcome)
CTK.publish ('^/wizard/vserver/moinmoin/2$', Common)
CTK.publish ('^/wizard/vserver/moinmoin/3$', Host)

# Rule
CTK.publish ('^/wizard/vserver/(\d+)/moinmoin$',   Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/moinmoin/2$', Common)
CTK.publish ('^/wizard/vserver/(\d+)/moinmoin/3$', WebDirectory)

# Common
CTK.publish (r'^%s$'%(URL_APPLY), Commit, method="POST", validation=VALS)
