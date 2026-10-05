# -*- coding: utf-8 -*-
#
# Cherokee-admin's Alfresco Community Edition Wizard
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
# 2009/10/xx: Adobe ColdFusion 9 / Cherokee 0.99.25
# 2010/04/15: Adobe ColdFusion 9 / Cherokee 0.99.41
#

import re
import urllib.request, urllib.parse, urllib.error
import CTK
import Wizard
from CTK.i18n import _, N_
import validations
from util import *

NOTE_WELCOME_H1 = N_("Welcome to the ColdFusion Wizard")
NOTE_WELCOME_P1 = N_('<a target="_blank" href="http://www.adobe.com/products/coldfusion/">Adobe ColdFusion®</a> enable developers to rapidly build, deploy, and maintain robust Internet applications for the enterprise.')
NOTE_WELCOME_P2 = N_('It allows developers to condense complex business logic into fewer lines of code.')

NOTE_COMMON_H1  = N_("ColdFusion Details")
NOTE_SOURCE     = N_('The pair IP:port of the server running ColdFusion. You can add more later to have the load balanced.')

NOTE_HOST_H1    = N_("New Virtual Server Details")
NOTE_HOST       = N_("Host name of the virtual server that is about to be created.")

NOTE_WEBDIR     = N_("Public web directory to access the project.")
NOTE_WEBDIR_H1  = N_("Public Web Directory")

ERROR_NO_SRC    = N_("ColdFusion is not running on the specified host.")

PREFIX          = 'tmp!wizard!coldfusion'
URL_APPLY       = r'/wizard/vserver/coldfusion/apply'

SOURCE = """
source!%(src_num)d!env_inherited = 0
source!%(src_num)d!type = host
source!%(src_num)d!nick = ColdFusion %(src_num)d
source!%(src_num)d!host = %(new_source)s
"""

CONFIG_VSERVER = SOURCE + """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = /dev/null

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!encoder!gzip = 1
%(vsrv_pre)s!rule!1!handler = proxy
%(vsrv_pre)s!rule!1!handler!balancer = ip_hash
%(vsrv_pre)s!rule!1!handler!balancer!source!1 = %(src_num)d
%(vsrv_pre)s!rule!1!handler!in_allow_keepalive = 1
%(vsrv_pre)s!rule!1!handler!in_preserve_host = 1
"""

"""
vserver!120!rule!100!handler!balancer = ip_hash
vserver!120!rule!100!handler!balancer!source!1 = 15
vserver!120!rule!100!handler!in_allow_keepalive = 1
vserver!120!rule!100!handler!in_preserve_host = 1
vserver!120!rule!100!match = default
vserver!120!rule!100!match!final = 1
"""

CONFIG_DIR = SOURCE + """
%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = %(webdir)s
%(rule_pre)s!encoder!gzip = 1
%(rule_pre)s!handler = proxy
%(rule_pre)s!handler!balancer = ip_hash
%(rule_pre)s!handler!balancer!source!1 = %(src_num)d
%(rule_pre)s!handler!in_allow_keepalive = 1
%(rule_pre)s!handler!in_preserve_host = 1
"""


class Commit:
    def Commit_VServer (self):
        # Incoming info
        new_host   = CTK.cfg.get_val(f'{PREFIX}!new_host')
        new_source = CTK.cfg.get_val(f'{PREFIX}!new_source')

        # Create the new Virtual Server
        vsrv_pre = CTK.cfg.get_next_entry_prefix('vserver')
        CTK.cfg[f'{vsrv_pre}!nick'] = new_host
        Wizard.CloneLogsCfg_Apply (f'{PREFIX}!logs_as_vsrv', vsrv_pre)

        # Locals
        src_num, src_pre = cfg_source_get_next ()

        # Add the new rules
        config = CONFIG_VSERVER %(locals())
        CTK.cfg.apply_chunk (config)

        # Clean up
        CTK.cfg.normalize (f'{vsrv_pre}!rule')
        CTK.cfg.normalize ('vserver')

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()


    def Commit_Rule (self):
        vsrv_num = CTK.cfg.get_val (f'{PREFIX}!vsrv_num')
        vsrv_pre = f'vserver!{vsrv_num}'

        # Incoming info
        webdir     = CTK.cfg.get_val(f'{PREFIX}!new_webdir')
        new_source = CTK.cfg.get_val(f'{PREFIX}!new_source')

        # Locals
        rule_pre = CTK.cfg.get_next_entry_prefix(f'{vsrv_pre}!rule')
        src_num, src_pre = cfg_source_get_next ()

        # Add the new rules
        config = CONFIG_DIR %(locals())
        CTK.cfg.apply_chunk (config)

        # Clean up
        CTK.cfg.normalize (f'{vsrv_pre}!rule')

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()


    def __call__ (self):
        if CTK.post.pop('final'):
            # Apply POST
            CTK.cfg_apply_post()

            # VServer or Rule?
            if CTK.cfg.get_val (f'{PREFIX}!vsrv_num'):
                return self.Commit_Rule()
            return self.Commit_VServer()

        return CTK.cfg_apply_post()


class WebDirectory:
    def __call__ (self):
        table = CTK.PropsTable()
        table.Add (_('Web Directory'), CTK.TextCfg (f'{PREFIX}!new_webdir', False, {'value': "/coldfusion", 'class': 'noauto'}), _(NOTE_WEBDIR))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML (f'<h2>{_(NOTE_WEBDIR_H1)}</h2>')
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class Host:
    def __call__ (self):
        table = CTK.PropsTable()
        table.Add (_('New Host Name'),    CTK.TextCfg (f'{PREFIX}!new_host', False, {'value': 'coldfusion.example.com', 'class': 'noauto'}), _(NOTE_HOST))

        submit  = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')
        submit += table

        cont  = CTK.Container()
        cont += CTK.RawHTML (f'<h2>{_(NOTE_HOST_H1)}</h2>')
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class Common:
    def __call__ (self):
        table = CTK.PropsTable()
        table.Add (_('Host'), CTK.TextCfg (f'{PREFIX}!new_source', False, {'value':"127.0.0.1:8500"}), _(NOTE_SOURCE))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML (f'<h2>{_(NOTE_COMMON_H1)}</h2>')
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevNext_Auto()
        return cont.Render().toStr()


class Welcome:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML (f'<h2>{_(NOTE_WELCOME_H1)}</h2>')
        cont += Wizard.Icon ('coldfusion', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML (f'<p>{_(NOTE_WELCOME_P1)}</p>')
        box += CTK.RawHTML (f'<p>{_(NOTE_WELCOME_P2)}</p>')
        box += Wizard.CookBookBox ('cookbook_coldfusion')
        cont += box

        # Send the VServer num if it is a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden(f'{PREFIX}!vsrv_num', tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


def is_coldfusion_host (host):
    host = validations.is_information_source (host)
    try:
        cf_host = urllib.request.urlopen(f"http://{host}")
    except:
        raise ValueError(_(ERROR_NO_SRC))

    headers = str(cf_host.headers).lower()
    cf_host.close()
    if not "jrun" in headers:
        raise ValueError(_(ERROR_NO_SRC))
    return host

VALS = [
    (f"{PREFIX}!new_host", validations.is_not_empty),
    (f"{PREFIX}!new_source", validations.is_not_empty),

    (f"{PREFIX}!new_host", validations.is_new_vserver_nick),
    (f"{PREFIX}!new_source", is_coldfusion_host),
]

# VServer
CTK.publish ('^/wizard/vserver/coldfusion$',   Welcome)
CTK.publish ('^/wizard/vserver/coldfusion/2$', Common)
CTK.publish ('^/wizard/vserver/coldfusion/3$', Host)

# Rule
CTK.publish ('^/wizard/vserver/(\\d+)/coldfusion$',   Welcome)
CTK.publish ('^/wizard/vserver/(\\d+)/coldfusion/2$', Common)
CTK.publish ('^/wizard/vserver/(\\d+)/coldfusion/3$', WebDirectory)

# Common
CTK.publish (rf'^{URL_APPLY}', Commit, method="POST", validation=VALS)
