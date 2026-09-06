# -*- coding: utf-8 -*-
#
# Symfony Wizard
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
# 2009/10/xx: Symfony 1.2.9  / Cherokee 0.99.25
# 2010/0x/xx: Symfony 1.2.10 / Cherokee 0.99.44
# 2010/0x/xx: Symfony 1.3.1  / Cherokee 0.99.44
# 2010/0x/xx: Symfony 1.4.1  / Cherokee 0.99.44
# 2010/04/19: Symfony 1.4.1  / Cherokee 0.99.45b
#

import os
import re
import CTK
import Wizard
from CTK.i18n import _, N_
import validations
from util import *

NOTE_WELCOME_H1 = N_("Welcome to the Symfony Wizard")
NOTE_WELCOME_P1 = N_('<a target="_blank" href="http://www.symfony-project.org/">Symfony</a>  is a full-stack framework, a library of cohesive classes written in PHP.')
NOTE_WELCOME_P2 = N_('It provides an architecture, components and tools for developers to build complex web applications faster.')

NOTE_LOCAL_H1   = N_("Application Source Code")
NOTE_LOCAL_DIR  = N_("Local directory where the Symfony source code is located. Example: /usr/share/php/data/symfony.")
NOTE_LOCAL_DROOT= N_("Path to the web folder of the Symfony project. (Example: /home/user/sf_project/web)")
ERROR_NO_SRC    = N_("Does not look like a Symfony source directory.")

NOTE_HOST_H1    = N_("New Virtual Server Details")
NOTE_HOST       = N_("Host name of the virtual server that is about to be created.")

NOTE_WEBDIR     = N_("Web directory where you want Symfony to be accessible. (Example: /symfony)")
NOTE_WEBDIR_H1  = N_("Public Web Directory")

PREFIX          = 'tmp!wizard!symfony'
URL_APPLY       = r'/wizard/vserver/symfony/apply'

CONFIG_DIR = """
%(pre_rule_plus2)s!document_root = %(local_src_dir)s/web/sf
%(pre_rule_plus2)s!encoder!gzip = 1
%(pre_rule_plus2)s!handler = file
%(pre_rule_plus2)s!handler!iocache = 1
%(pre_rule_plus2)s!match = directory
%(pre_rule_plus2)s!match!directory = %(web_dir)s/sf

%(pre_rule_plus1)s!document_root = %(document_root)s
%(pre_rule_plus1)s!match = directory
%(pre_rule_plus1)s!match!directory = %(web_dir)s
%(pre_rule_plus1)s!match!final = 0

# PHP rule

%(pre_rule_minus1)s!match = and
%(pre_rule_minus1)s!match!final = 1
%(pre_rule_minus1)s!match!left = directory
%(pre_rule_minus1)s!match!left!directory = %(web_dir)s
%(pre_rule_minus1)s!match!right = exists
%(pre_rule_minus1)s!match!right!iocache = 1
%(pre_rule_minus1)s!match!right!match_any = 1
%(pre_rule_minus1)s!match!right!match_index_files = 0
%(pre_rule_minus1)s!match!right!match_only_files = 1
%(pre_rule_minus1)s!handler = file
%(pre_rule_minus1)s!handler!iocache = 1

%(pre_rule_minus2)s!match = request
%(pre_rule_minus2)s!match!request = %(web_dir)s/(.+)
%(pre_rule_minus2)s!handler = redir
%(pre_rule_minus2)s!handler!rewrite!1!show = 0
%(pre_rule_minus2)s!handler!rewrite!1!substring = %(web_dir)s/index.php
"""

CONFIG_VSERVER = """
%(pre_vsrv)s!nick = %(host)s
%(pre_vsrv)s!document_root = %(document_root)s
%(pre_vsrv)s!directory_index = index.php,index.html

%(pre_rule_plus1)s!document_root = %(local_src_dir)s/web/sf
%(pre_rule_plus1)s!encoder!gzip = 1
%(pre_rule_plus1)s!handler = file
%(pre_rule_plus1)s!handler!iocache = 1
%(pre_rule_plus1)s!match = directory
%(pre_rule_plus1)s!match!directory = /sf

# PHP rule

%(pre_rule_minus1)s!handler = file
%(pre_rule_minus1)s!match = and
%(pre_rule_minus1)s!match!right = exists
%(pre_rule_minus1)s!match!right!match_any = 1
%(pre_rule_minus1)s!match!right!match_index_files = 1
%(pre_rule_minus1)s!match!right!match_only_files = 1
%(pre_rule_minus1)s!match!left = not
%(pre_rule_minus1)s!match!left!right = request
%(pre_rule_minus1)s!match!left!right!request = ^/$

%(pre_rule_minus2)s!handler = redir
%(pre_rule_minus2)s!handler!rewrite!1!regex = /.*
%(pre_rule_minus2)s!handler!rewrite!1!show = 0
%(pre_rule_minus2)s!handler!rewrite!1!substring = /index.php
%(pre_rule_minus2)s!match = default
"""

SRC_PATHS = [
    "/usr/share/php/data/symfony1.4",         # Debian, Fedora
    "/usr/share/php/data/symfony1.3",
    "/usr/share/php/data/symfony1.2",
    "/usr/share/php/data/symfony1.0",
    "/usr/share/php/data/symfony",
    "/usr/local/lib/php/data/symfony",        # Pear installation
    "/usr/share/pear/data/symfony",
    "/usr/share/pear/symfony",
]


class Commit:
    def Commit_VServer (self):
        # Create the new Virtual Server
        next = CTK.cfg.get_next_entry_prefix('vserver')
        CTK.cfg[f'{next}!nick'] = CTK.cfg.get_val(f'{PREFIX}!host')
        Wizard.CloneLogsCfg_Apply (f'{PREFIX}!logs_as_vsrv', next)

        # PHP
        php = CTK.load_module ('php', 'wizards')

        error = php.wizard_php_add (next)
        if error:
            return {'ret': 'error', 'errors': {'msg': error}}

        php_info = php.get_info (next)

        # Symfony
        props = cfg_get_surrounding_repls ('pre_rule', php_info['rule'])
        props['pre_vsrv']      = next
        props['host']          = CTK.cfg.get_val(f'{PREFIX}!host')
        props['local_src_dir'] = CTK.cfg.get_val(f'{PREFIX}!sources')
        props['document_root'] = CTK.cfg.get_val(f'{PREFIX}!document_root')

        config = CONFIG_VSERVER %(props)
        CTK.cfg.apply_chunk (config)

        # Clean up
        CTK.cfg.normalize (f'{next}!rule')
        CTK.cfg.normalize ('vserver')

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()

    def Commit_Rule (self):
        vsrv_num = CTK.cfg.get_val (f'{PREFIX}!vsrv_num')
        next = f'vserver!{vsrv_num}'

        # PHP
        php = CTK.load_module ('php', 'wizards')

        error = php.wizard_php_add (next)
        if error:
            return {'ret': 'error', 'errors': {'msg': error}}

        php_info = php.get_info (next)

        # Symfony
        props = cfg_get_surrounding_repls ('pre_rule', php_info['rule'])
        props['pre_vsrv']      = next
        props['web_dir']       = CTK.cfg.get_val(f'{PREFIX}!web_dir')
        props['local_src_dir'] = CTK.cfg.get_val(f'{PREFIX}!sources')
        props['document_root'] = CTK.cfg.get_val(f'{PREFIX}!document_root')

        config = CONFIG_DIR %(props)
        CTK.cfg.apply_chunk (config)

        # Clean up
        CTK.cfg.normalize (f'{next}!rule')

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
        table.Add (_('Web Directory'), CTK.TextCfg (f'{PREFIX}!web_dir', False, {'value': '/symfony', 'class': 'noauto'}), _(NOTE_WEBDIR))

        vsrv_num = CTK.cfg.get_val (f'{PREFIX}!vsrv_num')
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
        table.Add (_('New Host Name'),    CTK.TextCfg (f'{PREFIX}!host', False, {'value': 'www.example.com', 'class': 'noauto'}), _(NOTE_HOST))
        table.Add (_('Use Same Logs as'), Wizard.CloneLogsCfg(f'{PREFIX}!logs_as_vsrv'), _(Wizard.CloneLogsCfg.NOTE))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML (f'<h2>{_(NOTE_HOST_H1)}</h2>')
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class LocalSource:
    def __call__ (self):
        guessed_src = path_find_w_default (SRC_PATHS)

        table = CTK.PropsTable()
        table.Add (_('Project Source'), CTK.TextCfg (f'{PREFIX}!document_root', False, {'value': '/var/www'}), _(NOTE_LOCAL_DROOT))
        table.Add (_('Symfony Package'), CTK.TextCfg (f'{PREFIX}!sources', False, {'value': guessed_src}), _(NOTE_LOCAL_DIR))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML (f'<h2>{_(NOTE_LOCAL_H1)}</h2>')
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevNext_Auto()
        return cont.Render().toStr()


class PHP:
    def __call__ (self):
        php = CTK.load_module ('php', 'wizards')
        return php.External_FindPHP()


class Welcome:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML (f'<h2>{_(NOTE_WELCOME_H1)}</h2>')
        cont += Wizard.Icon ('symfony', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML (f'<p>{_(NOTE_WELCOME_P1)}</p>')
        box += CTK.RawHTML (f'<p>{_(NOTE_WELCOME_P2)}</p>')
        box += Wizard.CookBookBox ('cookbook_symfony')
        cont += box

        # Send the VServer num if it's a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden(f'{PREFIX}!vsrv_num', tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


def is_symfony_dir (path):
    path = validations.is_local_dir_exists (path)

    try:
        module_inc = os.path.join (path, 'bin/symfony')
        validations.is_local_file_exists (module_inc)
    except ValueError:
        module_inc = os.path.join (path, 'bin/check_configuration.php')
        validations.is_local_file_exists (module_inc)
    except:
        raise ValueError(_(ERROR_NO_SRC))

    return path

VALS = [
    (f'{PREFIX}!sources', validations.is_not_empty),
    (f'{PREFIX}!host', validations.is_not_empty),
    (f'{PREFIX}!web_dir', validations.is_not_empty),
    (f'{PREFIX}!document_root', validations.is_not_empty),

    (f'{PREFIX}!sources', is_symfony_dir),
    (f'{PREFIX}!host', validations.is_new_vserver_nick),
    (f'{PREFIX}!web_dir', validations.is_dir_formatted),
    (f'{PREFIX}!document_root', validations.is_local_dir_exists),
]

# VServer
CTK.publish ('^/wizard/vserver/symfony$',   Welcome)
CTK.publish ('^/wizard/vserver/symfony/2$', PHP)
CTK.publish ('^/wizard/vserver/symfony/3$', LocalSource)
CTK.publish ('^/wizard/vserver/symfony/4$', Host)

# Rule
CTK.publish ('^/wizard/vserver/(\\d+)/symfony$',   Welcome)
CTK.publish ('^/wizard/vserver/(\\d+)/symfony/2$', PHP)
CTK.publish ('^/wizard/vserver/(\\d+)/symfony/3$', LocalSource)
CTK.publish ('^/wizard/vserver/(\\d+)/symfony/4$', WebDirectory)

# Common
CTK.publish (rf'^{URL_APPLY}', Commit, method="POST", validation=VALS)
