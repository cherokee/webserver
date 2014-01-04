# -*- coding: utf-8 -*-
#
# Zend Wizard
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
# 2009/10/xx: Zend 1.9.3 / Cherokee 0.99.25
# 2010/04/19: Zend 1.9.4 / Cherokee 0.99.45b
#

import os
import re
import CTK
import Wizard
import validations
from util import *

NOTE_WELCOME_H1 = N_("Welcome to the Zend Wizard")
NOTE_WELCOME_P1 = N_('<a target="_blank" href="http://www.zend.com/">Zend</a> Framework is a simple, straightforward, open-source software framework for PHP 5 designed to eliminate the tedious details of coding and let you focus on the big picture.')
NOTE_WELCOME_P2 = N_('One of its strengths is the highly modular Model-View-Controller (MVC) design, making your code more reusable and easier to maintain.')

NOTE_LOCAL_H1   = N_("Application Source Code")
NOTE_LOCAL_DIR  = N_("Local directory where the Zend source code is located. Example: /usr/share/php/data/zend.")
ERROR_NO_SRC    = N_("Does not look like a Zend source directory.")

NOTE_HOST_H1    = N_("New Virtual Server Details")
NOTE_HOST       = N_("Host name of the virtual server that is about to be created.")

NOTE_WEBDIR     = N_("Web directory where you want Zend to be accessible. (Example: /zend)")
NOTE_WEBDIR_H1  = N_("Public Web Directory")

PREFIX          = 'tmp!wizard!zend'
URL_APPLY       = r'/wizard/vserver/zend/apply'

CONFIG_DIR = """
%(pre_rule_plus1)s!document_root = %(local_src_dir)s
%(pre_rule_plus1)s!match = directory
%(pre_rule_plus1)s!match!directory = %(web_dir)s
%(pre_rule_plus1)s!match!final = 0

# PHP rule

%(pre_rule_minus2)s!match = and
%(pre_rule_minus2)s!match!final = 1
%(pre_rule_minus2)s!match!left = directory
%(pre_rule_minus2)s!match!left!directory = %(web_dir)s
%(pre_rule_minus2)s!match!right = exists
%(pre_rule_minus2)s!match!right!iocache = 1
%(pre_rule_minus2)s!match!right!match_any = 1
%(pre_rule_minus2)s!match!right!match_index_files = 0
%(pre_rule_minus2)s!match!right!match_only_files = 1
%(pre_rule_minus2)s!handler = file
%(pre_rule_minus2)s!handler!iocache = 1

%(pre_rule_minus3)s!match = request
%(pre_rule_minus3)s!match!request = %(web_dir)s/.+
%(pre_rule_minus3)s!handler = redir
%(pre_rule_minus3)s!handler!rewrite!1!show = 0
%(pre_rule_minus3)s!handler!rewrite!1!substring = %(web_dir)s/index.php
"""

CONFIG_VSERVER = """
%(pre_vsrv)s!nick = %(host)s
%(pre_vsrv)s!document_root = %(local_src_dir)s
%(pre_vsrv)s!directory_index = index.php,index.html

# PHP rule

%(pre_rule_minus1)s!handler = common
%(pre_rule_minus1)s!match = fullpath
%(pre_rule_minus1)s!match!fullpath!1 = /

%(pre_rule_minus2)s!handler = common
%(pre_rule_minus2)s!match = exists
%(pre_rule_minus2)s!match!match_any = 1
%(pre_rule_minus2)s!match!match_index_files = 0
%(pre_rule_minus2)s!match!match_only_files = 1

%(pre_rule_minus3)s!handler = redir
%(pre_rule_minus3)s!handler!rewrite!1!regex = ^.*$
%(pre_rule_minus3)s!handler!rewrite!1!show = 0
%(pre_rule_minus3)s!handler!rewrite!1!substring = /index.php
%(pre_rule_minus3)s!match = default
%(pre_rule_minus3)s!match!final = 1
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
        if error:
            return {'ret': 'error', 'errors': {'msg': error}}

        php_info = php.get_info (next)

        # Zend
        props = cfg_get_surrounding_repls ('pre_rule', php_info['rule'])
        props['pre_vsrv']      = next
        props['host']          = CTK.cfg.get_val('%s!host'    %(PREFIX))
        props['local_src_dir'] = CTK.cfg.get_val('%s!sources' %(PREFIX))

        config = CONFIG_VSERVER %(props)
        CTK.cfg.apply_chunk (config)

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
        if error:
            return {'ret': 'error', 'errors': {'msg': error}}

        php_info = php.get_info (next)

        # Zend
        props = cfg_get_surrounding_repls ('pre_rule', php_info['rule'])
        props['pre_vsrv']      = next
        props['web_dir']       = CTK.cfg.get_val('%s!web_dir'   %(PREFIX))
        props['local_src_dir'] = CTK.cfg.get_val('%s!sources' %(PREFIX))

        config = CONFIG_DIR %(props)
        CTK.cfg.apply_chunk (config)

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
        table.Add (_('Web Directory'), CTK.TextCfg ('%s!web_dir'%(PREFIX), False, {'value': '/zend', 'class': 'noauto'}), _(NOTE_WEBDIR))

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
        table.Add (_('New Host Name'),    CTK.TextCfg ('%s!host'%(PREFIX), False, {'value': 'zend.example.com', 'class': 'noauto'}), _(NOTE_HOST))
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
        table = CTK.PropsTable()
        table.Add (_('Source Directory'), CTK.TextCfg ('%s!sources'%(PREFIX), False, {'value': '/var/www'}), _(NOTE_LOCAL_DIR))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_LOCAL_H1)))
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
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('zend', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P2)))
        box += Wizard.CookBookBox ('cookbook_zend')
        cont += box

        # Send the VServer num if it's a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


VALS = [
    ('%s!sources' %(PREFIX), validations.is_not_empty),
    ('%s!host'    %(PREFIX), validations.is_not_empty),
    ('%s!web_dir' %(PREFIX), validations.is_not_empty),

    ('%s!sources' %(PREFIX), validations.is_local_dir_exists),
    ('%s!host'    %(PREFIX), validations.is_new_vserver_nick),
    ('%s!web_dir' %(PREFIX), validations.is_dir_formatted),
]

# VServer
CTK.publish ('^/wizard/vserver/zend$',   Welcome)
CTK.publish ('^/wizard/vserver/zend/2$', PHP)
CTK.publish ('^/wizard/vserver/zend/3$', LocalSource)
CTK.publish ('^/wizard/vserver/zend/4$', Host)

# Rule
CTK.publish ('^/wizard/vserver/(\d+)/zend$',   Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/zend/2$', PHP)
CTK.publish ('^/wizard/vserver/(\d+)/zend/3$', LocalSource)
CTK.publish ('^/wizard/vserver/(\d+)/zend/4$', WebDirectory)

# Common
CTK.publish (r'^%s'%(URL_APPLY), Commit, method="POST", validation=VALS)
