# -*- coding: utf-8 -*-
#
# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
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
# 2010/04/12: Drupal 5.22 Cherokee 0.99.41
# 2010/04/12: Drupal 6.16 Cherokee 0.99.41
#

import os
import re
import CTK
import Wizard
import validations
from util import *

NOTE_WELCOME_H1 = N_("Welcome to the Drupal wizard")
NOTE_WELCOME_P1 = N_('<a target="_blank" href="http://drupal.org/">Drupal</a> is a free software package that allows an individual or a community of users to easily publish, manage and organize a wide variety of content on a website.')
NOTE_WELCOME_P2 = N_('Tens of thousands of people and organizations are using Drupal to power scores of different web sites.')
NOTE_LOCAL_H1   = N_("Application Source Code")
NOTE_LOCAL_DIR  = N_("Local directory where the Drupal source code is located. Example: /usr/share/drupal.")
NOTE_HOST_H1    = N_("New Virtual Server Details")
NOTE_HOST       = N_("Host name of the virtual server that is about to be created.")
NOTE_WEBDIR     = N_("Web directory where you want Drupal to be accessible. (Example: /blog)")
NOTE_WEBDIR_H1  = N_("Public Web Direcoty")
ERROR_NO_SRC    = N_("Does not look like a Drupal source directory.")

PREFIX    = 'tmp!wizard!drupal'
URL_APPLY = r'/wizard/vserver/drupal/apply'

SRC_PATHS = [
    "/usr/share/drupal7",          # Debian, Fedora
    "/usr/share/drupal",
    "/usr/share/drupal6",
    "/usr/share/drupal5",
    "/var/www/*/htdocs/drupal",    # Gentoo
    "/srv/www/htdocs/drupal",      # SuSE
    "/usr/local/www/data/drupal*", # BSD
    "/opt/local/www/data/drupal7", # MacPorts
    "/opt/local/www/data/drupal6",
    "/opt/local/www/data/drupal5"
]

CONFIG_VSERVER = """
%(pre_vsrv)s!nick = %(host)s
%(pre_vsrv)s!document_root = %(local_dir)s
%(pre_vsrv)s!directory_index = index.php,index.html

%(pre_rule_plus3)s!match = request
%(pre_rule_plus3)s!match!request = ^/([0-9]+)$
%(pre_rule_plus3)s!handler = redir
%(pre_rule_plus3)s!handler!rewrite!1!regex = ^/([0-9]+)$
%(pre_rule_plus3)s!handler!rewrite!1!show = 0
%(pre_rule_plus3)s!handler!rewrite!1!substring = /index.php?q=/node/$1

%(pre_rule_plus2)s!match = request
%(pre_rule_plus2)s!match!request = \.(engine|inc|info|install|module|profile|test|po|sh|.*sql|theme|tpl(\.php)?|xtmpl|svn-base)$|^(code-style\.pl|Entries.*|Repository|Root|Tag|Template|all-wcprops|entries|format)$
%(pre_rule_plus2)s!handler = custom_error
%(pre_rule_plus2)s!handler!error = 403

%(pre_rule_plus1)s!match = fullpath
%(pre_rule_plus1)s!match!fullpath!1 = /
%(pre_rule_plus1)s!handler = redir
%(pre_rule_plus1)s!handler!rewrite!1!show = 0
%(pre_rule_plus1)s!handler!rewrite!1!substring = /index.php

# IMPORTANT: The PHP rule comes here

%(pre_rule_minus1)s!match = exists
%(pre_rule_minus1)s!match!iocache = 1
%(pre_rule_minus1)s!match!match_any = 1
%(pre_rule_minus1)s!match!match_index_files = 0
%(pre_rule_minus1)s!match!match_only_files = 1
%(pre_rule_minus1)s!handler = file

%(pre_rule_minus2)s!match = default
%(pre_rule_minus2)s!handler = redir
%(pre_rule_minus2)s!handler!rewrite!1!show = 0
%(pre_rule_minus2)s!handler!rewrite!1!regex = ^/(.*)\?(.*)$
%(pre_rule_minus2)s!handler!rewrite!1!substring = /index.php?q=$1&$2
%(pre_rule_minus2)s!handler!rewrite!2!show = 0
%(pre_rule_minus2)s!handler!rewrite!2!regex = ^/(.*)$
%(pre_rule_minus2)s!handler!rewrite!2!substring = /index.php?q=$1
"""

CONFIG_DIR = """
%(pre_rule_plus4)s!match = request
%(pre_rule_plus4)s!match!request = ^%(web_dir)s/([0-9]+)$
%(pre_rule_plus4)s!handler = redir
%(pre_rule_plus4)s!handler!rewrite!1!regex = ^%(web_dir)s/([0-9]+)$
%(pre_rule_plus4)s!handler!rewrite!1!show = 0
%(pre_rule_plus4)s!handler!rewrite!1!substring = %(web_dir)s/index.php?q=/node/$1

%(pre_rule_plus3)s!match = request
%(pre_rule_plus3)s!match!request = ^%(web_dir)s/$
%(pre_rule_plus3)s!handler = redir
%(pre_rule_plus3)s!handler!rewrite!1!show = 0
%(pre_rule_plus3)s!handler!rewrite!1!substring = %(web_dir)s/index.php

%(pre_rule_plus2)s!match = directory
%(pre_rule_plus2)s!match!directory = %(web_dir)s
%(pre_rule_plus2)s!match!final = 0
%(pre_rule_plus2)s!document_root = %(local_dir)s

%(pre_rule_plus1)s!match = and
%(pre_rule_plus1)s!match!left = directory
%(pre_rule_plus1)s!match!left!directory = %(web_dir)s
%(pre_rule_plus1)s!match!right = request
%(pre_rule_plus1)s!match!right!request = \.(engine|inc|info|install|module|profile|test|po|sh|.*sql|theme|tpl(\.php)?|xtmpl|svn-base)$|^(code-style\.pl|Entries.*|Repository|Root|Tag|Template|all-wcprops|entries|format)$
%(pre_rule_plus1)s!handler = custom_error
%(pre_rule_plus1)s!handler!error = 403

# IMPORTANT: The PHP rule comes here

%(pre_rule_minus1)s!match = and
%(pre_rule_minus1)s!match!left = directory
%(pre_rule_minus1)s!match!left!directory = %(web_dir)s
%(pre_rule_minus1)s!match!right = exists
%(pre_rule_minus1)s!match!right!iocache = 1
%(pre_rule_minus1)s!match!right!match_any = 1
%(pre_rule_minus1)s!match!right!match_index_files = 0
%(pre_rule_minus1)s!match!right!match_only_files = 1
%(pre_rule_minus1)s!handler = file

%(pre_rule_minus2)s!match = directory
%(pre_rule_minus2)s!match!directory = %(web_dir)s
%(pre_rule_minus2)s!handler = redir
%(pre_rule_minus2)s!handler!rewrite!1!show = 0
%(pre_rule_minus2)s!handler!rewrite!1!regex = ^/(.*)\?(.*)$
%(pre_rule_minus2)s!handler!rewrite!1!substring = %(web_dir)s/index.php?q=$1&$2
%(pre_rule_minus2)s!handler!rewrite!2!show = 0
%(pre_rule_minus2)s!handler!rewrite!2!regex = ^/(.*)$
%(pre_rule_minus2)s!handler!rewrite!2!substring = %(web_dir)s/index.php?q=$1
"""


class Commit:
    def Commit_VServer (self):
        # Create the new Virtual Server
        next = CTK.cfg.get_next_entry_prefix('vserver')
        CTK.cfg['%s!nick'%(next)] = CTK.cfg.get_val('%s!host'%(PREFIX))
        Wizard.CloneLogsCfg_Apply ('%s!logs_as_vsrv'%(PREFIX), next)

        # PHP
        php = CTK.load_module ('php', 'wizards')
        error = php.wizard_php_add (next)
        php_info = php.get_info (next)

        # Drupal
        props = cfg_get_surrounding_repls ('pre_rule', php_info['rule'])
        props['pre_vsrv']  = next
        props['host']      = CTK.cfg.get_val('%s!host'      %(PREFIX))
        props['local_dir'] = CTK.cfg.get_val('%s!local_dir' %(PREFIX))

        config = CONFIG_VSERVER %(props)
        CTK.cfg.apply_chunk (config)

        # Fixes Drupal bug for multilingual content
        CTK.cfg['%s!encoder!gzip' %(php_info['rule'])] = '0'

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(next))
        CTK.cfg.normalize ('vserver')

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()


    def Commit_Rule (self):
        vsrv_num = CTK.cfg.get_val ('%s!vsrv_num'%(PREFIX))
        next = 'vserver!%s' %(vsrv_num)

        # PHP
        php = CTK.load_module ('php', 'wizards')
        error = php.wizard_php_add (next)
        php_info = php.get_info (next)

        # Drupal
        props = cfg_get_surrounding_repls ('pre_rule', php_info['rule'])
        props['pre_vsrv']  = next
        props['web_dir']   = CTK.cfg.get_val('%s!web_dir'   %(PREFIX))
        props['local_dir'] = CTK.cfg.get_val('%s!local_dir' %(PREFIX))

        config = CONFIG_DIR %(props)
        CTK.cfg.apply_chunk (config)

        # Fixes Drupal bug for multilingual content
        CTK.cfg['%s!encoder!gzip' %(php_info['rule'])] = '0'

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(next))

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
        table.Add (_('Web Directory'), CTK.TextCfg ('%s!web_dir'%(PREFIX), False, {'value': '/blog', 'class': 'noauto'}), _(NOTE_WEBDIR))

        vsrv_num = CTK.cfg.get_val ('%s!vsrv_num'%(PREFIX))
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
        table.Add (_('New Host Name'),    CTK.TextCfg ('%s!host'%(PREFIX), False, {'value': 'www.example.com', 'class': 'noauto'}), _(NOTE_HOST))
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
        table.Add (_('Drupal Local Directory'), CTK.TextCfg ('%s!local_dir'%(PREFIX), False, {'value': guessed_src}), _(NOTE_LOCAL_DIR))

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
        cont += Wizard.Icon ('drupal', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P2)))
        cont += box

        # Send the VServer num if it's a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


def is_drupal_dir (path):
    path = validations.is_local_dir_exists (path)
    module_inc = os.path.join (path, 'includes/module.inc')
    try:
        validations.is_local_file_exists (module_inc)
    except:
        raise ValueError, _(ERROR_NO_SRC)
    return path


VALS = [
    ('%s!local_dir'%(PREFIX), validations.is_not_empty),
    ('%s!host'     %(PREFIX), validations.is_not_empty),
    ('%s!web_dir'  %(PREFIX), validations.is_not_empty),

    ('%s!local_dir'%(PREFIX), is_drupal_dir),
    ('%s!host'     %(PREFIX), validations.is_new_vserver_nick),
    ('%s!web_dir'  %(PREFIX), validations.is_dir_formatted),
]

# VServer
CTK.publish ('^/wizard/vserver/drupal$',   Welcome)
CTK.publish ('^/wizard/vserver/drupal/2$', LocalSource)
CTK.publish ('^/wizard/vserver/drupal/3$', Host)

# Rule
CTK.publish ('^/wizard/vserver/(\d+)/drupal$',   Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/drupal/2$', LocalSource)
CTK.publish ('^/wizard/vserver/(\d+)/drupal/3$', WebDirectory)

# Common
CTK.publish (r'^%s$'%(URL_APPLY), Commit, method="POST", validation=VALS)
