# -*- coding: utf-8 -*-
#
# Cherokee-admin's GlassFish Wizard
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
# 2009/10/xx: GlassFish v2 / Cherokee 0.99.36b
# 2010/04/15: GlassFish v2 / Cherokee 0.99.41
#

import re
import CTK
import Wizard
import validations
from util import *

NOTE_WELCOME_H1 = N_("Welcome to the GlassFish Wizard")
NOTE_WELCOME_P1 = N_('<a target="_blank" href="http://glassfish.org/">GlassFish</a> is the first compatible implementation of the Java EE 6 platform specification.')
NOTE_WELCOME_P2 = N_('This lightweight, flexible, and open-source application server enables organizations to not only leverage the new capabilities introduced within the Java EE 6 specification, but also to add to their existing capabilities through a faster and more streamlined development and deployment cycle.')

NOTE_COMMON_H1  = N_("GlassFish Project")
NOTE_COMMON_SRC = N_('Hostname or IP of the server running GlassFish. You can add more later to have the load balanced.')
NOTE_COMMON_PRT = N_('Port running the service in said host. (Example: 8080)')

NOTE_HOST       = N_("Host name of the virtual server that is about to be created.")
NOTE_HOST_H1    = N_("New Virtual Server Details")

NOTE_WEBDIR     = N_("Public web directory to access the project.")
NOTE_WEBDIR_H1  = N_("Public Web Directory")

PREFIX    = 'tmp!wizard!glassfish'

URL_APPLY      = r'/wizard/vserver/glassfish/apply'

SOURCE = """
source!%(src_num)d!env_inherited = 0
source!%(src_num)d!type = host
source!%(src_num)d!nick = Glassfish %(src_num)d
source!%(src_num)d!host = %(src_host)s:%(src_port)d
"""

CONFIG_VSERVER = SOURCE + """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = /dev/null

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!encoder!gzip = 1
%(vsrv_pre)s!rule!1!handler = proxy
%(vsrv_pre)s!rule!1!handler!balancer = round_robin
%(vsrv_pre)s!rule!1!handler!balancer!source!1 = %(src_num)d
%(vsrv_pre)s!rule!1!handler!in_allow_keepalive = 1
%(vsrv_pre)s!rule!1!handler!in_preserve_host = 0
"""

CONFIG_DIR = SOURCE + """
%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = %(webdir)s
%(rule_pre)s!encoder!gzip = 1
%(rule_pre)s!handler = proxy
%(rule_pre)s!handler!balancer = round_robin
%(rule_pre)s!handler!balancer!source!1 = %(src_num)d
%(rule_pre)s!handler!in_allow_keepalive = 1
%(rule_pre)s!handler!in_preserve_host = 0
"""


class Commit:
    def Commit_VServer (self):
        # Incoming info
        new_host = CTK.cfg.get_val('%s!new_host'%(PREFIX))
        src_host = CTK.cfg.get_val('%s!new_src_host'%(PREFIX))
        src_port = int(CTK.cfg.get_val('%s!new_src_port'%(PREFIX)))

        # Create the new Virtual Server
        vsrv_pre = CTK.cfg.get_next_entry_prefix('vserver')
        CTK.cfg['%s!nick'%(vsrv_pre)] = new_host
        Wizard.CloneLogsCfg_Apply ('%s!logs_as_vsrv'%(PREFIX), vsrv_pre)

        # Locals
        src_num, src_pre = cfg_source_get_next ()

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
        webdir   = CTK.cfg.get_val('%s!new_webdir'%(PREFIX))
        src_host = CTK.cfg.get_val('%s!new_src_host'%(PREFIX))
        src_port = int(CTK.cfg.get_val('%s!new_src_port'%(PREFIX)))

        # Locals
        rule_pre = CTK.cfg.get_next_entry_prefix('%s!rule'%(vsrv_pre))
        src_num, src_pre = cfg_source_get_next ()

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
        table.Add (_('Web Directory'), CTK.TextCfg ('%s!new_webdir'%(PREFIX), False, {'value': "/glassfish", 'class': 'noauto'}), _(NOTE_WEBDIR))

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
        table.Add (_('New Host Name'),    CTK.TextCfg ('%s!new_host'%(PREFIX), False, {'value': 'glassfish.example.com', 'class': 'noauto'}), _(NOTE_HOST))

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
        table = CTK.PropsTable()
        table.Add (_('Source host'), CTK.TextCfg ('%s!new_src_host'%(PREFIX), False, {'value':'localhost'}), _(NOTE_COMMON_SRC))
        table.Add (_('Source port'), CTK.TextCfg ('%s!new_src_port'%(PREFIX), False, {'value':8080}),        _(NOTE_COMMON_PRT))

        submit = CTK.Submitter (URL_APPLY)
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
        cont += Wizard.Icon ('glassfish', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P2)))
        box += Wizard.CookBookBox ('cookbook_glassfish')
        cont += box

        # Send the VServer num if it is a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


VALS = [
    ("%s!new_host"     %(PREFIX), validations.is_not_empty),
    ("%s!new_webdir"   %(PREFIX), validations.is_not_empty),
    ("%s!new_src_port" %(PREFIX), validations.is_not_empty),
    ("%s!new_src_host" %(PREFIX), validations.is_not_empty),

    ("%s!new_host"     %(PREFIX), validations.is_new_vserver_nick),
    ("%s!new_webdir"   %(PREFIX), validations.is_dir_formatted),
    ("%s!new_src_port" %(PREFIX), validations.is_tcp_port),
]

# VServer
CTK.publish ('^/wizard/vserver/glassfish$',   Welcome)
CTK.publish ('^/wizard/vserver/glassfish/2$', Common)
CTK.publish ('^/wizard/vserver/glassfish/3$', Host)

# Rule
CTK.publish ('^/wizard/vserver/(\d+)/glassfish$',   Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/glassfish/2$', Common)
CTK.publish ('^/wizard/vserver/(\d+)/glassfish/3$', WebDirectory)

# Common
CTK.publish (r'^%s'%(URL_APPLY), Commit, method="POST", validation=VALS)
