# -*- coding: utf-8 -*-
#
# Sugar CRM Wizard
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
# 2009/10/xx: Sugar CE 5.5.0 / Cherokee 0.99.25
# 2010/04/16: Sugar CE 5.5.1 / Cherokee 0.99.41
#

import os
import re
import CTK
import Wizard
import validations
from util import *

NOTE_WELCOME_H1 = N_("Welcome to the Sugar CRM Wizard")
NOTE_WELCOME_P1 = N_('<a target="_blank" href="http://www.sugarcrm.com/">Sugar</a> enables organizations to efficiently organize, populate, and maintain information on all aspects of their customer relationships.')
NOTE_WELCOME_P2 = N_('It provides integrated management of corporate information on customer accounts and contacts, sales leads and opportunities, plus activities such as calls, meetings, and assigned tasks.')

NOTE_LOCAL_H1   = N_("Application Source Code")
NOTE_LOCAL_DIR  = N_("Local directory where the Sugar CRM source code is located. Example: /usr/share/sugar.")
ERROR_NO_SRC    = N_("Does not look like a Sugar CRM source directory.")

NOTE_HOST_H1    = N_("New Virtual Server Details")
NOTE_HOST       = N_("Host name of the virtual server that is about to be created.")

NOTE_WEBDIR     = N_("Web directory where you want Sugar CRM to be accessible. (Example: /sugar)")
NOTE_WEBDIR_H1  = N_("Public Web Direcoty")

PREFIX          = 'tmp!wizard!sugar'
URL_APPLY       = r'/wizard/vserver/sugar/apply'

CONFIG_DIR = """
# The PHP rule comes here

%(pre_rule_minus1)s!match = directory
%(pre_rule_minus1)s!match!directory = %(web_dir)s
%(pre_rule_minus1)s!match!final = 0
%(pre_rule_minus1)s!document_root = %(local_src_dir)s

%(pre_rule_minus2)s!handler = redir
%(pre_rule_minus2)s!handler!rewrite!1!show = 1
%(pre_rule_minus2)s!handler!rewrite!1!substring = %(web_dir)s/index.php
%(pre_rule_minus2)s!match = request
%(pre_rule_minus2)s!match!request = ^/.*/.*\.php

%(pre_rule_minus3)s!handler = redir
%(pre_rule_minus3)s!handler!rewrite!1!show = 1
%(pre_rule_minus3)s!handler!rewrite!1!substring = %(web_dir)s/index.php
%(pre_rule_minus3)s!match = request
%(pre_rule_minus3)s!match!request = emailmandelivery.php

%(pre_rule_minus4)s!handler = redir
%(pre_rule_minus4)s!handler!rewrite!1!show = 0
%(pre_rule_minus4)s!handler!rewrite!1!substring = %(web_dir)s/log_file_restricted.html
%(pre_rule_minus4)s!match = request
%(pre_rule_minus4)s!match!request = ^/(.*\.log.*|not_imported_.*txt)
"""

CONFIG_VSERVER = """
%(pre_vsrv)s!nick = %(host)s
%(pre_vsrv)s!document_root = %(local_src_dir)s
%(pre_vsrv)s!directory_index = index.php,index.html

%(pre_rule_plus3)s!handler = redir
%(pre_rule_plus3)s!handler!rewrite!1!show = 1
%(pre_rule_plus3)s!handler!rewrite!1!substring = /index.php
%(pre_rule_plus3)s!match = request
%(pre_rule_plus3)s!match!request = ^/.*/.*\.php

%(pre_rule_plus2)s!handler = redir
%(pre_rule_plus2)s!handler!rewrite!1!show = 1
%(pre_rule_plus2)s!handler!rewrite!1!substring = /index.php
%(pre_rule_plus2)s!match = request
%(pre_rule_plus2)s!match!request = emailmandelivery.php

%(pre_rule_plus1)s!handler = redir
%(pre_rule_plus1)s!handler!rewrite!1!show = 0
%(pre_rule_plus1)s!handler!rewrite!1!substring = /log_file_restricted.html
%(pre_rule_plus1)s!match = request
%(pre_rule_plus1)s!match!request = ^/(.*\.log.*|not_imported_.*txt)

# The PHP rule comes here

%(pre_rule_minus1)s!handler = common
%(pre_rule_minus1)s!handler!iocache = 0
%(pre_rule_minus1)s!match = default
"""

SRC_PATHS = [
    "/usr/share/sugar",         # Debian, Fedora
    "/var/www/*/htdocs/sugar",  # Gentoo
    "/srv/www/htdocs/sugar",    # SuSE
    "/usr/local/www/data/sugar" # BSD
]


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

        # Sugar CRM
        props = cfg_get_surrounding_repls ('pre_rule', php_info['rule'])
        props['pre_vsrv']      = next
        props['host']          = CTK.cfg.get_val('%s!host'    %(PREFIX))
        props['local_src_dir'] = CTK.cfg.get_val('%s!sources' %(PREFIX))

        config = CONFIG_VSERVER %(props)
        CTK.cfg.apply_chunk (config)

        # Common static
        Wizard.AddUsualStaticFiles(props['pre_rule_plus9'])

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

        # Sugar CRM
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
        table.Add (_('Web Directory'), CTK.TextCfg ('%s!web_dir'%(PREFIX), False, {'value': '/sugar', 'class': 'noauto'}), _(NOTE_WEBDIR))

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
        table.Add (_('Sugar Local Directory'), CTK.TextCfg ('%s!sources'%(PREFIX), False, {'value': guessed_src}), _(NOTE_LOCAL_DIR))

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
        cont += Wizard.Icon ('sugar', {'class': 'wizard-descr'})
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


def is_sugar_dir (path):
    path = validations.is_local_dir_exists (path)
    module_inc = os.path.join (path, 'include/entryPoint.php')
    try:
        validations.is_local_file_exists (module_inc)
    except:
        raise ValueError, _(ERROR_NO_SRC)
    return path

VALS = [
    ('%s!sources' %(PREFIX), validations.is_not_empty),
    ('%s!host'    %(PREFIX), validations.is_not_empty),
    ('%s!web_dir' %(PREFIX), validations.is_not_empty),

    ('%s!sources' %(PREFIX), is_sugar_dir),
    ('%s!host'    %(PREFIX), validations.is_new_vserver_nick),
    ('%s!web_dir' %(PREFIX), validations.is_dir_formatted)
]

# VServer
CTK.publish ('^/wizard/vserver/sugar$',   Welcome)
CTK.publish ('^/wizard/vserver/sugar/2$', LocalSource)
CTK.publish ('^/wizard/vserver/sugar/3$', Host)

# Rule
CTK.publish ('^/wizard/vserver/(\d+)/sugar$',   Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/sugar/2$', LocalSource)
CTK.publish ('^/wizard/vserver/(\d+)/sugar/3$', WebDirectory)

# Common
CTK.publish (r'^%s$'%(URL_APPLY), Commit, method="POST", validation=VALS)
