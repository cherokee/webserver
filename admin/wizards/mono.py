# -*- coding: utf-8 -*-
#
# Cherokee-admin's .NET/Mono Wizard
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
# 2010/04/15: mono-xsp2 2.4.2-1/ Cherokee 0.99.41
#


import re
import CTK
import Wizard
import validations
from util import *

NOTE_WELCOME_H1   = N_("Welcome to the mono Wizard")
NOTE_WELCOME_P1   = N_('<a target="_blank" href="http://www.mono-project.com/">Mono</a> is a cross-platform, open-source .NET development framework.')
NOTE_WELCOME_P2   = N_('It is an open implementation of C# and the CLR that is binary compatible with Microsoft.NET.')

NOTE_LOCAL_H1     = N_('Mono Project')
NOTE_MONO_DIR     = N_("Local path to the Mono based project.")
NOTE_MONO_BIN     = N_("Local path to the FastCGI Mono server binary.")

NOTE_HOST_H1    = N_("New Virtual Server Details")
NOTE_HOST       = N_("Host name of the virtual server that is about to be created.")
NOTE_DROOT      = N_("Document root (for non Mono related files).")

NOTE_WEBDIR     = N_("Web directory under which the Mono application will be published.")
NOTE_WEBDIR_H1  = N_("Public Web Directory")

ERROR_NO_MONO   = N_("Directory doesn't look like a Mono based project.")

PREFIX          = 'tmp!wizard!mono'
URL_APPLY       = r'/wizard/vserver/mono/apply'

SOURCE = """
source!%(src_num)d!type = interpreter
source!%(src_num)d!nick = Mono %(src_num)d
source!%(src_num)d!host = 127.0.0.1:%(src_port)d
source!%(src_num)d!interpreter = %(mono_bin)s --socket=tcp --address=127.0.0.1 --port=%(src_port)d --applications=%(webdir)s:%(mono_dir)s
"""

CONFIG_VSERVER = SOURCE + """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = %(document_root)s
%(vsrv_pre)s!directory_index = %(index)s

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!match!final = 1
%(vsrv_pre)s!rule!1!encoder!gzip = 1
%(vsrv_pre)s!rule!1!handler = fcgi
%(vsrv_pre)s!rule!1!handler!check_file = 1
%(vsrv_pre)s!rule!1!handler!balancer = round_robin
%(vsrv_pre)s!rule!1!handler!balancer!source!1 = %(src_num)d
"""

CONFIG_DIR = SOURCE + """
%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = %(webdir)s
%(rule_pre)s!encoder!gzip = 1
%(rule_pre)s!handler = fcgi
%(rule_pre)s!handler!check_file = 1
%(rule_pre)s!handler!balancer = round_robin
%(rule_pre)s!handler!balancer!source!1 = %(src_num)d
"""

DEFAULT_BINS  = ['fastcgi-mono-server2','fastcgi-mono-server']

DEFAULT_PATHS = ['/usr/bin',
                 '/usr/sfw/bin',
                 '/usr/gnu/bin',
                 '/opt/local/bin']

class Commit:
    def Commit_VServer (self):
        # Incoming info
        new_host      = CTK.cfg.get_val('%s!new_host'%(PREFIX))
        document_root = CTK.cfg.get_val('%s!document_root'%(PREFIX))
        mono_dir      = CTK.cfg.get_val('%s!mono_dir'%(PREFIX))
        webdir        = '/'
        mono_bin      = CTK.cfg.get_val('%s!mono_bin'%(PREFIX))

        # Index
        listdir = os.listdir (get_real_path (document_root))
        files = {'index.aspx': 'index.aspx', 'default.aspx':'default.aspx'}
        for x in listdir:
            files[x.lower()] = x
        index = '%s,%s' % (files['index.aspx'], files['default.aspx'])

        # Create the new Virtual Server
        vsrv_pre = CTK.cfg.get_next_entry_prefix('vserver')
        CTK.cfg['%s!nick'%(vsrv_pre)] = new_host
        Wizard.CloneLogsCfg_Apply ('%s!logs_as_vsrv'%(PREFIX), vsrv_pre)

        # Locals
        src_num, src_pre  = cfg_source_get_next ()
        src_port          = cfg_source_find_free_port ()
        if not mono_bin:
            mono_bin      = path_find_binary (DEFAULT_BINS,
                                              extra_dirs  = DEFAULT_PATHS)

        # Add the new rules
        config = CONFIG_VSERVER %(locals())
        CTK.cfg.apply_chunk (config)

        # Usual Static Files
        Wizard.AddUsualStaticFiles ("%s!rule!500" % (vsrv_pre))

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(vsrv_pre))
        CTK.cfg.normalize ('vserver')

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()


    def Commit_Rule (self):
        vsrv_num = CTK.cfg.get_val ('%s!vsrv_num'%(PREFIX))
        vsrv_pre = 'vserver!%s' %(vsrv_num)

        # Incoming info
        mono_dir = CTK.cfg.get_val('%s!mono_dir'%(PREFIX))
        webdir   = CTK.cfg.get_val('%s!new_webdir'%(PREFIX))
        mono_bin = CTK.cfg.get_val('%s!mono_bin'%(PREFIX))

        # Locals
        rule_pre = CTK.cfg.get_next_entry_prefix('%s!rule'%(vsrv_pre))
        src_port = cfg_source_find_free_port ()
        src_num, src_pre  = cfg_source_get_next ()
        if not mono_bin:
            mono_bin = path_find_binary (DEFAULT_BINS,
                                         extra_dirs  = DEFAULT_PATHS)

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
        table.Add (_('Web Directory'), CTK.TextCfg ('%s!new_webdir'%(PREFIX), False, {'value': '/project', 'class': 'noauto'}), _(NOTE_WEBDIR))

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
        table.Add (_('New Host Name'),    CTK.TextCfg ('%s!new_host'%(PREFIX), False, {'value': 'www.example.com', 'class': 'noauto'}), _(NOTE_HOST))
        table.Add (_('Document Root'),    CTK.TextCfg ('%s!document_root'%(PREFIX), False, {'value': os_get_document_root(), 'class': 'noauto'}), _(NOTE_DROOT))
        table.Add (_('Use Same Logs as'), Wizard.CloneLogsCfg('%s!logs_as_vsrv'%(PREFIX)), _(Wizard.CloneLogsCfg.NOTE))

        submit  = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')
        submit += table

        cont  = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_HOST_H1)))
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class LocalSource:
    def __call__ (self):
        mono_bin = path_find_binary (DEFAULT_BINS,
                                     extra_dirs  = DEFAULT_PATHS)

        table = CTK.PropsTable()
        table.Add (_('Project Directory'), CTK.TextCfg ('%s!mono_dir'%(PREFIX), False), _(NOTE_MONO_DIR))
        if not mono_bin:
            table.Add (_('Mono Server Binary'), CTK.TextCfg ('%s!mono_bin'%(PREFIX), False), _(NOTE_MONO_BIN))

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
        cont += Wizard.Icon ('mono', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P2)))
        box += Wizard.CookBookBox ('cookbook_mono')
        cont += box

        # Send the VServer num if it is a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


def is_mono_dir (path):
    path = validations.is_local_dir_exists (path)

    real_path = get_real_path (path)
    files = [x.lower() for x in os.listdir(real_path)]
    if 'index.aspx' in files or 'default.aspx' in files:
        return path

    raise ValueError, _(ERROR_NO_MONO)

VALS = [
    ('%s!mono_dir'     %(PREFIX), validations.is_not_empty),
    ('%s!mono_bin'     %(PREFIX), validations.is_not_empty),
    ('%s!new_host'     %(PREFIX), validations.is_not_empty),
    ('%s!new_webdir'   %(PREFIX), validations.is_not_empty),
    ('%s!document_root'%(PREFIX), validations.is_not_empty),

    ("%s!mono_dir"     %(PREFIX), is_mono_dir),
    ("%s!mono_bin"     %(PREFIX), validations.is_local_file_exists),
    ("%s!new_host"     %(PREFIX), validations.is_new_vserver_nick),
    ("%s!new_webdir"   %(PREFIX), validations.is_dir_formatted),
    ("%s!document_root"%(PREFIX), validations.is_local_dir_exists),
]

# VServer
CTK.publish ('^/wizard/vserver/mono$',   Welcome)
CTK.publish ('^/wizard/vserver/mono/2$', LocalSource)
CTK.publish ('^/wizard/vserver/mono/3$', Host)

# Rule
CTK.publish ('^/wizard/vserver/(\d+)/mono$',   Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/mono/2$', LocalSource)
CTK.publish ('^/wizard/vserver/(\d+)/mono/3$', WebDirectory)

# Common
CTK.publish (r'^%s'%(URL_APPLY), Commit, method="POST", validation=VALS)
