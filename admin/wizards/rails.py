# -*- coding: utf-8 -*-
#
# Cherokee-admin's Ruby on Rails Wizard
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
# 2010/09/13: Rails 3.0.0 / Cherokee 1.0.9b
# 2010/04/14: Rails 2.2.3 / Cherokee 0.99.41
#

import os
import re
import CTK
import Wizard
import validations

from util import *

NOTE_WELCOME_H1 = N_("Welcome to the Ruby on Rails wizard")
NOTE_WELCOME_P1 = N_('<a target="_blank" href="http://rubyonrails.org/">Ruby on Rails</a> is an open-source web framework optimized for programmer happines and sustainable productivity.')
NOTE_WELCOME_P2 = N_('It lets you write beautiful code by favoring convention over configuration.')

NOTE_LOCAL_H1   = N_("Ruby on Rails Project")
NOTE_HOST_H1    = N_("New Virtual Server")
NOTE_WEBDIR_H1  = N_("Web Directory")

NOTE_ROR_DIR    = N_("Local path to the Ruby on Rails based project.")
NOTE_NEW_HOST   = N_("Name of the new domain that will be created.")
NOTE_NEW_DIR    = N_("Directory of the web directory where the Ruby on Rails project will live in.")
NOTE_ENV        = N_("Value of the RAILS_ENV variable.")
NOTE_METHOD     = N_("The proxy setting is recommended, but FastCGI can also be used if spawn-fcgi is available.")

ERROR_DISPATCH  = N_("<p>Even though the directory looks like a Ruby on Rails project, the public/dispatch.fcgi file wasn't found.</p>")
ERROR_EXAMPLE   = N_("<p>However a <b>public/dispatch.fcgi.example</b> file is present, so you might want to rename it.</p>")
ERROR_RAILS23   = N_("<p>If you are using Rails >= 2.3.0, you will have to execute the following command from the project directory in order to add the missing file:</p><p><pre>rake rails:update:generate_dispatchers</pre></p>")
ERROR_NO_ROR    = N_("It does not look like a Ruby on Rails based project directory.")
ERROR_NO_DROOT  = N_("The document root directory does not exist.")

PREFIX          = 'tmp!wizard!rails'
URL_APPLY       = r'/wizard/vserver/rails/apply'

ROR_CHILD_PROCS = 3
DEFAULT_BINS    = ['spawn-fcgi']

RAILS_ENV = [
    ('production',  N_('Production')),
    ('test',        N_('Test')),
    ('development', N_('Development')),
    ('',            N_('Empty'))
]

RAILS_METHOD = [
    ('proxy',  N_('HTTP proxy')),
    ('fcgi',   N_('FastCGI'))
]

SOURCE = """
source!%(src_num)d!type = interpreter
source!%(src_num)d!nick = RoR %(new_host)s, instance %(src_instance)d
source!%(src_num)d!host = 127.0.0.1:%(src_port)d
source!%(src_num)d!env_inherited = 0
"""

SOURCE_FCGI = """
source!%(src_num)d!interpreter = spawn-fcgi -n -d %(ror_dir)s -f %(ror_dir)s/public/dispatch.fcgi -p %(src_port)d
"""

SOURCE_PROXY = """
source!%(src_num)d!interpreter = %(ror_dir)s/script/server -p %(src_port)d
"""

SOURCE_PROXY3 = """
source!%(src_num)d!interpreter = %(ror_dir)s/script/rails s -p %(src_port)d -P tmp/pids/cherokee_$(src_port)d.pid
"""

SOURCE_PROXY4 = """
source!%(src_num)d!interpreter = %(ror_dir)s/bin/rails s -p %(src_port)d
"""

SOURCE_ENV = """
source!%(src_num)d!env!RAILS_ENV = %(ror_env)s
"""

CONFIG_VSRV = """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = %(ror_dir)s/public
%(vsrv_pre)s!directory_index = index.html

%(vsrv_pre)s!rule!10!match = exists
%(vsrv_pre)s!rule!10!match!match_any = 1
%(vsrv_pre)s!rule!10!match!match_only_files = 1
%(vsrv_pre)s!rule!10!match!match_index_files = 0
%(vsrv_pre)s!rule!10!handler = common
%(vsrv_pre)s!rule!10!expiration = time
%(vsrv_pre)s!rule!10!expiration!time = 7d

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!encoder!gzip = 1
"""

CONFIG_VSRV_FCGI = """
%(vsrv_pre)s!rule!1!handler = fcgi
%(vsrv_pre)s!rule!1!handler!error_handler = 1
%(vsrv_pre)s!rule!1!handler!check_file = 0
%(vsrv_pre)s!rule!1!handler!balancer = round_robin
"""

CONFIG_VSRV_PROXY = """
%(vsrv_pre)s!rule!1!handler = proxy
%(vsrv_pre)s!rule!1!handler!balancer = round_robin
%(vsrv_pre)s!rule!1!handler!in_allow_keepalive = 1
"""

CONFIG_VSRV_CHILD = """
%(vsrv_pre)s!rule!1!handler!balancer!source!%(src_instance)d = %(src_num)d
"""

CONFIG_RULES = """
%(rule_pre_plus2)s!match = directory
%(rule_pre_plus2)s!match!directory = %(webdir)s
%(rule_pre_plus2)s!match!final = 0
%(rule_pre_plus2)s!document_root = %(ror_dir)s/public

%(rule_pre_plus1)s!match = and
%(rule_pre_plus1)s!match!left = directory
%(rule_pre_plus1)s!match!left!directory = %(webdir)s
%(rule_pre_plus1)s!match!right = exists
%(rule_pre_plus1)s!match!right!match_any = 1
%(rule_pre_plus1)s!match!right!match_only_files = 1
%(rule_pre_plus1)s!match!right!match_index_files = 0
%(rule_pre_plus1)s!handler = common
%(rule_pre_plus1)s!expiration = time
%(rule_pre_plus1)s!expiration!time = 7d

%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = %(webdir)s
%(rule_pre)s!encoder!gzip = 1
"""

CONFIG_RULES_FCGI = """
%(rule_pre)s!handler = fcgi
%(rule_pre)s!handler!error_handler = 1
%(rule_pre)s!handler!check_file = 0
%(rule_pre)s!handler!balancer = round_robin
"""

CONFIG_RULES_PROXY = """
%(rule_pre)s!handler = proxy
%(rule_pre)s!handler!balancer = round_robin
%(rule_pre)s!handler!in_allow_keepalive = 1
"""

CONFIG_RULES_CHILD = """
%(rule_pre)s!handler!balancer!source!%(src_instance)d = %(src_num)d
"""


class Commit:
    def Commit_VServer (self):
        # Create the new Virtual Server
        vsrv_pre = CTK.cfg.get_next_entry_prefix('vserver')
        CTK.cfg['%s!nick'%(vsrv_pre)] = CTK.cfg.get_val('%s!host'%(PREFIX))
        Wizard.CloneLogsCfg_Apply ('%s!logs_as_vsrv'%(PREFIX), vsrv_pre)

        # Incoming info
        ror_dir    = CTK.cfg.get_val('%s!ror_dir'%(PREFIX))
        new_host   = CTK.cfg.get_val('%s!new_host'%(PREFIX))
        ror_env    = CTK.cfg.get_val('%s!ror_env'%(PREFIX))
        ror_method = CTK.cfg.get_val('%s!ror_method'%(PREFIX))

        # Locals
        src_num, src_pre = cfg_source_get_next ()

        # Deployment method distinction
        CONFIG = CONFIG_VSRV
        SRC    = SOURCE
        if ror_method == 'fcgi':
            # Check whether dispatch.fcgi is present
            dispatcher = Dispatcher()
            if not dispatcher._fcgi_ok():
                return {'ret':'error'}

            CONFIG += CONFIG_VSRV_FCGI
            SRC    += SOURCE_FCGI
        else:
            CONFIG += CONFIG_VSRV_PROXY
            SRC    += get_proxy_source (ror_dir)

        # Add the new main rules
        config = CONFIG % (locals())

        # Add the Information Sources
        free_port = cfg_source_find_free_port()
        for i in range(ROR_CHILD_PROCS):
            src_instance = i + 1
            src_port     = i + free_port
            config += SRC % (locals())
            if ror_env:
                config += SOURCE_ENV % (locals())
            config += CONFIG_VSRV_CHILD % (locals())
            src_num += 1

        # Apply the configuration
        CTK.cfg.apply_chunk (config)

        # Usual Static Files
        Wizard.AddUsualStaticFiles ("%s!rule!500" % (vsrv_pre))

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(vsrv_pre))
        CTK.cfg.normalize ('vserver')

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()


    def Commit_Rule (self):
        # Incoming info
        ror_dir    = CTK.cfg.get_val('%s!ror_dir'%(PREFIX))
        webdir     = CTK.cfg.get_val('%s!new_webdir'%(PREFIX))
        ror_env    = CTK.cfg.get_val('%s!ror_env'%(PREFIX))
        ror_method = CTK.cfg.get_val('%s!ror_method'%(PREFIX))
        vsrv_num   = CTK.cfg.get_val('%s!vsrv_num'%(PREFIX))

        # Locals
        vsrv_pre           = 'vserver!%s'%(vsrv_num)
        rule_num, rule_pre = cfg_vsrv_rule_get_next (vsrv_pre)
        src_num,  src_pre  = cfg_source_get_next ()
        new_host           = CTK.cfg.get_val ("%s!nick"%(vsrv_pre))
        rule_pre_plus2     = "%s!rule!%d" % (vsrv_pre, rule_num + 2)
        rule_pre_plus1     = "%s!rule!%d" % (vsrv_pre, rule_num + 1)

        # Deployment method distinction
        CONFIG = CONFIG_RULES
        SRC    = SOURCE
        if ror_method == 'fcgi':
            # Check whether dispatch.fcgi is present
            dispatcher = Dispatcher()
            if not dispatcher._fcgi_ok():
                return {'ret':'error'}

            CONFIG += CONFIG_RULES_FCGI
            SRC    += SOURCE_FCGI
        else:
            CONFIG += CONFIG_RULES_PROXY
            SRC    += get_proxy_source (ror_dir)

        # Add the new rules
        config = CONFIG % (locals())

        # Add the Information Sources
        free_port = cfg_source_find_free_port()
        for i in range(ROR_CHILD_PROCS):
            src_instance = i + 1
            src_port     = i + free_port
            config += SRC % (locals())
            if ror_env:
                config += SOURCE_ENV % (locals())
            config += CONFIG_RULES_CHILD % (locals())
            src_num += 1

        # Apply the configuration
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
        table.Add (_('Web Directory'), CTK.TextCfg ('%s!new_webdir'%(PREFIX), False, {'value': '/project', 'class': 'noauto'}), _(NOTE_NEW_DIR))

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
        table.Add (_('New Host Name'),    CTK.TextCfg ('%s!new_host'%(PREFIX), False, {'value': 'www.example.com', 'class': 'noauto'}), _(NOTE_NEW_HOST))
        table.Add (_('Use Same Logs as'), Wizard.CloneLogsCfg('%s!logs_as_vsrv'%(PREFIX)), _(Wizard.CloneLogsCfg.NOTE))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_HOST_H1)))
        cont += Dispatcher ()
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class Dispatcher (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        ror_method = CTK.cfg.get_val('%s!ror_method'%(PREFIX))
        if ror_method != 'fcgi':
            return

        if self._fcgi_ok():
            return

        if self.errors.has_key('dispatch.fcgi'):
            if self.errors.has_key('dispatch.fcgi.example'):
                message = _(ERROR_DISPATCH) + _(ERROR_EXAMPLE)
            else:
                message = _(ERROR_DISPATCH) + _(ERROR_RAILS23)

        self += CTK.Notice ('important-information', CTK.RawHTML (message))

    def _fcgi_ok (self):
        ror_dir = CTK.cfg.get_val('%s!ror_dir'%(PREFIX))

        # Check whether dispatch.fcgi is present
        if not os.path.exists (os.path.join (ror_dir, "public/dispatch.fcgi")):
            self.errors = {'dispatch.fcgi': 'Not found'}
            if os.path.exists (os.path.join (ror_dir, "public/dispatch.fcgi.example")):
                self.errors['dispatch.fcgi.example'] = True
            return False
        return True


class LocalSource:
    def __call__ (self):
        # Trim deployment options if needed
        if not path_find_binary (DEFAULT_BINS) and len(RAILS_METHOD) == 2:
            RAILS_METHOD.remove(('fcgi', 'FastCGI'))

        submit  = CTK.Submitter (URL_APPLY)
        table   = CTK.PropsTable()
        submit += table

        table.Add (_('Project Directory'), CTK.TextCfg ('%s!ror_dir'%(PREFIX), False), _(NOTE_ROR_DIR))
        table.Add (_('RAILS_ENV environment'), CTK.ComboCfg ('%s!ror_env'%(PREFIX), trans_options(RAILS_ENV), {'class': 'noauto'}), _(NOTE_ENV))
        table.Add (_('Deployment method'), CTK.ComboCfg ('%s!ror_method'%(PREFIX), trans_options(RAILS_METHOD), {'class': 'noauto'}), _(NOTE_METHOD))

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_LOCAL_H1)))
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevNext_Auto()
        return cont.Render().toStr()


class Welcome:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('rails', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P2)))
        box += Wizard.CookBookBox ('cookbook_ror')
        cont += box

        # Send the VServer num if it's a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


def is_ror_dir(path):
    path = validations.is_local_dir_exists (path)

    ror2_bin = os.path.join(path, "script/server")
    ror3_bin = os.path.join(path, "script/rails")
    ror4_bin = os.path.join(path, "bin/rails")

    if os.path.exists(ror2_bin):
        return path
    elif os.path.exists(ror3_bin):
        return path
    elif os.path.exists(ror4_bin):
        return path
    else:
        raise ValueError, _(ERROR_NO_ROR)

def get_ror_version(path):
    path = validations.is_local_dir_exists (path)

    ror2_bin = os.path.join(path, "script/server")
    ror3_bin = os.path.join(path, "script/rails")
    ror4_bin = os.path.join(path, "bin/rails")

    if os.path.exists(ror2_bin):
        return 2
    elif os.path.exists(ror3_bin):
        return 3
    elif os.path.exists(ror4_bin):
        return 4


def get_proxy_source(ror_dir):
    if get_ror_version(ror_dir) == 2:
        return SOURCE_PROXY
    elif get_ror_version(ror_dir) == 3:
        return SOURCE_PROXY3
    elif get_ror_version(ror_dir) == 4:
        return SOURCE_PROXY4


VALS = [
    ('%s!ror_dir' %(PREFIX), validations.is_not_empty),
    ('%s!new_host'%(PREFIX), validations.is_not_empty),

    ("%s!ror_dir" %(PREFIX), is_ror_dir),
    ("%s!new_host"%(PREFIX), validations.is_new_vserver_nick)
]

# VServer
CTK.publish ('^/wizard/vserver/rails$',   Welcome)
CTK.publish ('^/wizard/vserver/rails/2$', LocalSource)
CTK.publish ('^/wizard/vserver/rails/3$', Host)

# Rule
CTK.publish ('^/wizard/vserver/(\d+)/rails$',   Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/rails/2$', LocalSource)
CTK.publish ('^/wizard/vserver/(\d+)/rails/3$', WebDirectory)

# Common
CTK.publish (r'^%s'%(URL_APPLY), Commit, method="POST", validation=VALS)

