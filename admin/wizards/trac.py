# -*- coding: utf-8 -*-
#
# Cherokee-admin's Trac Wizard
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
# 2009/xx/xx: Trac 0.11.1 Cherokee 0.99.25
# 2010/04/12: Trac 0.11.7 Cherokee 0.99.41
#

import os
import re
import CTK
import Wizard
import validations
from util import *

NOTE_WELCOME_H1 = N_("Welcome to the Trac wizard")
NOTE_WELCOME_P1 = N_('<a target="_blank" href="http://trac.edgewall.org/">Trac</a> is an enhanced wiki and issue tracking system for software development projects. Trac uses a minimalistic approach to web-based software project management.')
NOTE_WELCOME_P2 = N_('It provides an interface to Subversion (or other version control systems), an integrated Wiki and convenient reporting facilities.')
NOTE_LOCAL_H1   = N_("Application Source Code")
NOTE_HOST_H1    = N_("New Virtual Server Details")
NOTE_HOST       = N_("Host name of the virtual server that is about to be created.")
NOTE_TRAC_PROJECT = N_("Local path to the Trac project.")
NOTE_TRAC_DATA  = N_("Local path to the Trac installation. (Example: /usr/share/trac)")

ERROR_NO_PROJECT= N_("It does not look like a Trac based project directory.")
ERROR_NO_DATA   = N_("It does not look like a Trac installation.")

PREFIX    = 'tmp!wizard!trac'

URL_APPLY      = r'/wizard/vserver/trac/apply'

# TCP port value is automatically asigned to one currently not in use

SOURCE = """
source!%(src_num)d!type = interpreter
source!%(src_num)d!nick = Trac %(src_num)d
source!%(src_num)d!host = 127.0.0.1:%(src_port)d
source!%(src_num)d!interpreter = tracd --single-env --daemonize --protocol=scgi --hostname=%(localhost)s --port=%(src_port)s %(trac_project)s
"""

CONFIG_VSERVER = SOURCE + """
%(pre_vsrv)s!nick = %(new_host)s
%(pre_vsrv)s!document_root = /dev/null

%(pre_vsrv)s!rule!10!match = directory
%(pre_vsrv)s!rule!10!match!directory = /chrome/common
%(pre_vsrv)s!rule!10!document_root = %(trac_data)s/htdocs
%(pre_vsrv)s!rule!10!handler = file
%(pre_vsrv)s!rule!10!expiration = time
%(pre_vsrv)s!rule!10!expiration!time = 7d

%(pre_vsrv)s!rule!1!match = default
%(pre_vsrv)s!rule!1!encoder!gzip = 1
%(pre_vsrv)s!rule!1!handler = scgi
%(pre_vsrv)s!rule!1!handler!change_user = 0
%(pre_vsrv)s!rule!1!handler!check_file = 0
%(pre_vsrv)s!rule!1!handler!error_handler = 0
%(pre_vsrv)s!rule!1!handler!pass_req_headers = 1
%(pre_vsrv)s!rule!1!handler!xsendfile = 0
%(pre_vsrv)s!rule!1!handler!balancer = round_robin
%(pre_vsrv)s!rule!1!handler!balancer!source!1 = %(src_num)d
"""

SRC_PATHS = [
    "/usr/share/pyshared/trac",
    "/usr/share/trac"
]


class Commit:
    def Commit_VServer (self):
        # Create the new Virtual Server
        next = CTK.cfg.get_next_entry_prefix('vserver')
        CTK.cfg['%s!nick'%(next)] = CTK.cfg.get_val('%s!host'%(PREFIX))
        Wizard.CloneLogsCfg_Apply ('%s!logs_as_vsrv'%(PREFIX), next)

        # Trac
        props = {}
        props['pre_vsrv']     = next
        props['new_host']     = CTK.cfg.get_val('%s!new_host'     %(PREFIX))
        props['trac_data']    = CTK.cfg.get_val('%s!trac_data'    %(PREFIX)).rstrip('/')
        props['trac_project'] = CTK.cfg.get_val('%s!trac_project' %(PREFIX))
        props['src_num'], x   = cfg_source_get_next()
        props['src_port']     = cfg_source_find_free_port()
        props['localhost']    = cfg_source_get_localhost_addr()

        config = CONFIG_VSERVER %(props)
        CTK.cfg.apply_chunk (config)

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(next))
        CTK.cfg.normalize ('vserver')

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()


    def __call__ (self):
        if CTK.post.pop('final'):
            CTK.cfg_apply_post()
            return self.Commit_VServer()

        return CTK.cfg_apply_post()


class Host:
    def __call__ (self):
        table = CTK.PropsTable()
        table.Add (_('New Host Name'),    CTK.TextCfg ('%s!new_host'%(PREFIX), False, {'value': 'www.example.com', 'class': 'noauto'}), _(NOTE_HOST))
        table.Add (_('Use Same Logs as'), Wizard.CloneLogsCfg('%s!logs_as_vsrv'%(PREFIX)), _(Wizard.CloneLogsCfg.NOTE))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_HOST_H1)))
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class LocalSource:
    def __call__ (self):
        guessed_src = path_find_w_default (SRC_PATHS)

        table = CTK.PropsTable()
        table.Add (_('Trac Local Directory'), CTK.TextCfg ('%s!trac_data'%(PREFIX), False, {'value': guessed_src}), _(NOTE_TRAC_DATA))
        table.Add (_('Project Directory'),    CTK.TextCfg ('%s!trac_project'%(PREFIX), False, {'value': os_get_document_root()}), _(NOTE_TRAC_PROJECT))
        submit = CTK.Submitter (URL_APPLY)
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_LOCAL_H1)))
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevNext_Auto()
        return cont.Render().toStr()


class Welcome:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('trac', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P2)))
        box += Wizard.CookBookBox ('cookbook_trac')
        cont += box

        # Sent the VServer num if it's a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


def is_trac_data (path):
    path = validations.is_local_dir_exists (path)
    manage = os.path.join (path, "htdocs")

    try:
        validations.is_local_file_exists (manage)
    except:
        raise ValueError, _(ERROR_NO_DATA)
    return path

def is_trac_project (path):
    path = validations.is_local_dir_exists (path)
    manage = os.path.join (path, "conf/trac.ini")

    try:
        validations.is_local_file_exists (manage)
    except:
        raise ValueError, _(ERROR_NO_PROJECT)
    return path

VALS = [
    ('%s!trac_data'   %(PREFIX), validations.is_not_empty),
    ('%s!trac_project'%(PREFIX), validations.is_not_empty),
    ('%s!new_host'    %(PREFIX), validations.is_not_empty),
    ('%s!new_webdir'  %(PREFIX), validations.is_not_empty),

    ("%s!trac_data"   %(PREFIX), is_trac_data),
    ("%s!trac_project"%(PREFIX), is_trac_project),
    ("%s!new_host"    %(PREFIX), validations.is_new_vserver_nick),
    ("%s!new_webdir"  %(PREFIX), validations.is_dir_formatted)
]

# VServer
CTK.publish ('^/wizard/vserver/trac$',   Welcome)
CTK.publish ('^/wizard/vserver/trac/2$', LocalSource)
CTK.publish ('^/wizard/vserver/trac/3$', Host)
CTK.publish (r'^%s$'%(URL_APPLY), Commit, method="POST", validation=VALS)
