# -*- coding: utf-8 -*-
#
# Cherokee-admin's WordPress wizard
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
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
# 2010/04/12: WordPress 2.9.2 Cherokee 0.99.41
#

import os
import re
import CTK
import Wizard
import validations
from util import *

NOTE_WELCOME_H1 = N_("Welcome to the WordPress wizard")
NOTE_WELCOME_P1 = N_('<a target="_blank" href="http://wordpress.org/">WordPress</a> is a state-of-the-art publishing platform with a focus on aesthetics, web standards, and usability. WordPress is both free and priceless at the same time.')
NOTE_WELCOME_P2 = N_('More simply, WordPress is what you use when you want to work with your blogging software, not fight it.')
NOTE_LOCAL_H1   = N_("Application Source Code")
NOTE_LOCAL_DIR  = N_("Local directory where the WordPress source code is located. Example: /usr/share/wordpress.")
NOTE_HOST_H1    = N_("New Virtual Server Details")
NOTE_HOST       = N_("Host name of the virtual server that is about to be created.")
NOTE_WEBDIR     = N_("Web directory where you want WordPress to be accessible. (Example: /blog)")
NOTE_WEBDIR_H1  = N_("Public Web Direcoty")
ERROR_NO_SRC    = N_("Does not look like a WordPress source directory.")

PREFIX    = 'tmp!wizard!wordpress'
URL_APPLY = r'/wizard/vserver/wordpress/apply'

CONFIG_DIR = """
%(pre_rule_minus1)s!match = fullpath
%(pre_rule_minus1)s!match!fullpath!1 = %(web_dir)s/wp-admin/
%(pre_rule_minus1)s!handler = redir
%(pre_rule_minus1)s!handler!rewrite!1!regex = (.*)/
%(pre_rule_minus1)s!handler!rewrite!1!show = 0
%(pre_rule_minus1)s!handler!rewrite!1!substring = $1/index.php

%(pre_rule_minus2)s!document_root = %(local_dir)s
%(pre_rule_minus2)s!match = directory
%(pre_rule_minus2)s!match!directory = %(web_dir)s
%(pre_rule_minus2)s!match!final = 0

%(pre_rule_minus3)s!match = and
%(pre_rule_minus3)s!match!final = 1
%(pre_rule_minus3)s!match!left = directory
%(pre_rule_minus3)s!match!left!directory = %(web_dir)s
%(pre_rule_minus3)s!match!right = exists
%(pre_rule_minus3)s!match!right!iocache = 1
%(pre_rule_minus3)s!match!right!match_any = 1
%(pre_rule_minus3)s!match!right!match_index_files = 0
%(pre_rule_minus3)s!match!right!match_only_files = 1
%(pre_rule_minus3)s!handler = file
%(pre_rule_minus3)s!handler!iocache = 1

%(pre_rule_minus4)s!match = request
%(pre_rule_minus4)s!match!request = %(web_dir)s/(.+)
%(pre_rule_minus4)s!handler = redir
%(pre_rule_minus4)s!handler!rewrite!1!show = 0
%(pre_rule_minus4)s!handler!rewrite!1!substring = %(web_dir)s/index.php?/$1
"""

CONFIG_VSERVER = """
%(pre_vsrv)s!document_root = %(local_dir)s
%(pre_vsrv)s!nick = %(host)s
%(pre_vsrv)s!directory_index = index.php,index.html

# The PHP rule comes here

%(pre_rule_minus2)s!match = fullpath
%(pre_rule_minus2)s!match!fullpath!1 = /
%(pre_rule_minus2)s!match!fullpath!2 = /wp-admin/
%(pre_rule_minus2)s!handler = redir
%(pre_rule_minus2)s!handler!rewrite!1!show = 0
%(pre_rule_minus2)s!handler!rewrite!1!regex = (.*)/
%(pre_rule_minus2)s!handler!rewrite!1!substring = $1/index.php

%(pre_rule_minus3)s!match = exists
%(pre_rule_minus3)s!match!iocache = 1
%(pre_rule_minus3)s!match!match_any = 1
%(pre_rule_minus3)s!match!match_only_files = 1
%(pre_rule_minus3)s!handler = file
%(pre_rule_minus3)s!handler!iocache = 1

%(pre_vsrv)s!rule!1!match = default
%(pre_vsrv)s!rule!1!handler = redir
%(pre_vsrv)s!rule!1!handler!rewrite!1!show = 0
%(pre_vsrv)s!rule!1!handler!rewrite!1!regex = /(.+)
%(pre_vsrv)s!rule!1!handler!rewrite!1!substring = /index.php?/$1
"""

SRC_PATHS = [
    "/usr/share/wordpress",          # Debian, Fedora
    "/var/www/*/htdocs/wordpress",   # Gentoo
    "/srv/www/htdocs/wordpress",     # SuSE
    "/usr/local/www/data/wordpress", # BSD
    "/opt/local/www/wordpress"       # MacPorts
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

        # WordPress
        props = cfg_get_surrounding_repls ('pre_rule', php_info['rule'])
        props['pre_vsrv']  = next
        props['host']      = CTK.cfg.get_val('%s!host'      %(PREFIX))
        props['local_dir'] = CTK.cfg.get_val('%s!local_dir' %(PREFIX))

        config = CONFIG_VSERVER %(props)
        CTK.cfg.apply_chunk (config)
        Wizard.AddUsualStaticFiles(props['pre_rule_minus1'])

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

        # WordPress
        props = cfg_get_surrounding_repls ('pre_rule', php_info['rule'])
        props['pre_vsrv']  = next
        props['web_dir']   = CTK.cfg.get_val('%s!web_dir'   %(PREFIX))
        props['local_dir'] = CTK.cfg.get_val('%s!local_dir' %(PREFIX))

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
        table.Add (_('Web Directory'), CTK.TextCfg ('%s!web_dir'%(PREFIX), False, {'value': '/blog', 'class': 'noauto'}), _(NOTE_WEBDIR))

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
        table.Add (_('WordPress Local Directory'), CTK.TextCfg ('%s!local_dir'%(PREFIX), False, {'value': guessed_src}), _(NOTE_LOCAL_DIR))

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
        cont += Wizard.Icon ('wordpress', {'class': 'wizard-descr'})
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


def is_wordpress_dir (path):
    path = validations.is_local_dir_exists (path)
    module_inc = os.path.join (path, 'wp-login.php')
    try:
        validations.is_local_file_exists (module_inc)
    except:
        raise ValueError, _(ERROR_NO_SRC)
    return path


VALS = [
    ('%s!local_dir'%(PREFIX), validations.is_not_empty),
    ('%s!host'     %(PREFIX), validations.is_not_empty),
    ('%s!web_dir'  %(PREFIX), validations.is_not_empty),

    ('%s!local_dir'%(PREFIX), is_wordpress_dir),
    ('%s!host'     %(PREFIX), validations.is_new_vserver_nick),
    ('%s!web_dir'  %(PREFIX), validations.is_dir_formatted)
]

# VServer
CTK.publish ('^/wizard/vserver/wordpress$',   Welcome)
CTK.publish ('^/wizard/vserver/wordpress/2$', LocalSource)
CTK.publish ('^/wizard/vserver/wordpress/3$', Host)

# Rule
CTK.publish ('^/wizard/vserver/(\d+)/wordpress$',   Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/wordpress/2$', LocalSource)
CTK.publish ('^/wizard/vserver/(\d+)/wordpress/3$', WebDirectory)

# Common
CTK.publish (r'^%s$'%(URL_APPLY), Commit, method="POST", validation=VALS)
