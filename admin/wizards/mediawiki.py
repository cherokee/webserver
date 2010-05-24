# -*- coding: utf-8 -*-
#
# Cherokee-admin's MediaWiki wizard
#
# Authors:
#      Taher Shihadeh <taher@octality.com>
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
# 2010/05/14: MediaWiki 1.15.3 Cherokee 1.0.0
#

import os
import re
import CTK
import Wizard
import validations
from util import *

NOTE_WELCOME_H1 = N_("Welcome to the Mediawiki wizard")
NOTE_WELCOME_P1 = N_('<a target="_blank" href="http://mediawiki.org/">MediaWiki</a> is a free software wiki  package written in PHP, originally for use on Wikipedia.')
NOTE_WELCOME_P2 = N_('It is now used by several other projects of the non-profit Wikimedia Foundation and by many other wikis.')
NOTE_LOCAL_H1   = N_("Application Source Code")
NOTE_LOCAL_DIR  = N_("Local directory where the MediaWiki source code is located. Example: /usr/share/mediawiki.")
NOTE_HOST_H1    = N_("New Virtual Server Details")
NOTE_HOST       = N_("Host name of the virtual server that is about to be created.")
NOTE_WEBDIR     = N_("Web directory where you want MediaWiki to be accessible. (Example: /wiki)")
NOTE_WEBDIR_H1  = N_("Public Web Directory")
ERROR_NO_SRC    = N_("Does not look like a MediaWiki source directory.")

NOTE_AUXDIR     = N_("Internal web directory used by MediaWiki. It usually is /w.")

NOTE_SETTINGS_H1= N_("Changes to LocalSettings.php")
NOTE_SETTINGS_P1= N_("Once you complete the installation of MediaWiki, LocalSettings.php will be created.")
NOTE_SETTINGS_P2= N_("For this wizard to be effective you will have to add the following changes to that file.")

PREFIX    = 'tmp!wizard!mediawiki'
URL_APPLY = r'/wizard/vserver/mediawiki/apply'

CONFIG_DIR = """
%(pre_rule_plus3)s!document_root = %(local_dir)s
%(pre_rule_plus3)s!match = directory
%(pre_rule_plus3)s!match!directory = %(aux_dir)s
%(pre_rule_plus3)s!match!final = 0

%(pre_rule_plus2)s!handler = redir
%(pre_rule_plus2)s!handler!rewrite!1!show = 1
%(pre_rule_plus2)s!handler!rewrite!1!substring = %(aux_dir)s/index.php
%(pre_rule_plus2)s!match = fullpath
%(pre_rule_plus2)s!match!fullpath!1 = %(web_dir)s
%(pre_rule_plus2)s!match!fullpath!2 = %(web_dir)s/

%(pre_rule_plus1)s!handler = redir
%(pre_rule_plus1)s!handler!rewrite!1!show = 0
%(pre_rule_plus1)s!handler!rewrite!1!substring = %(aux_dir)s/index.php?/$1
%(pre_rule_plus1)s!match = request
%(pre_rule_plus1)s!match!request = %(web_dir)s/(.+)
"""

CONFIG_VSERVER = """
%(pre_vsrv)s!document_root = %(local_dir)s
%(pre_vsrv)s!nick = %(host)s
%(pre_vsrv)s!directory_index = index.php,index.html

# The PHP rule comes here

%(pre_vsrv)s!rule!1!match = default
%(pre_vsrv)s!rule!1!handler = file
""" + CONFIG_DIR

LOCAL_SETTINGS = """
$wgScriptPath       = "%(aux_dir)s";
$wgScript           = "$wgScriptPath/index.php";
$wgRedirectScript   = "$wgScriptPath/redirect.php";
$wgArticlePath      = "%(web_dir)s/$1";
$wgUsePathInfo      = true;
"""

SRC_PATHS = [
    "/var/lib/mediawiki",            # Debian
    "/usr/share/mediawiki",          # Debian, Fedora
    "/var/www/*/htdocs/mediawiki",   # Gentoo
    "/srv/www/htdocs/mediawiki",     # SuSE
    "/usr/local/www/data/mediawiki", # BSD
    "/opt/local/www/mediawiki"       # MacPorts
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
        if error:
            return {'ret': 'error', 'errors': {'msg': error}}

        php_info = php.get_info (next)

        # MediaWiki
        props = cfg_get_surrounding_repls ('pre_rule', php_info['rule'])
        props['pre_vsrv']  = next
        props['host']      = CTK.cfg.get_val('%s!host'      %(PREFIX))
        props['local_dir'] = CTK.cfg.get_val('%s!local_dir' %(PREFIX))
        props['web_dir']   = CTK.cfg.get_val('%s!web_dir'   %(PREFIX))
        props['aux_dir']   = CTK.cfg.get_val('%s!aux_dir'   %(PREFIX))

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
        if error:
            return {'ret': 'error', 'errors': {'msg': error}}

        php_info = php.get_info (next)

        # MediaWiki
        props = cfg_get_surrounding_repls ('pre_rule', php_info['rule'])
        props['pre_vsrv']  = next
        props['web_dir']   = CTK.cfg.get_val('%s!web_dir'   %(PREFIX))
        props['local_dir'] = CTK.cfg.get_val('%s!local_dir' %(PREFIX))
        props['aux_dir']   = CTK.cfg.get_val('%s!aux_dir'   %(PREFIX))

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


class ShowLocalSettings:
    def __call__ (self):
        print 1

        web_dir = CTK.cfg.get_val('%s!web_dir'%(PREFIX))
        aux_dir = CTK.cfg.get_val('%s!aux_dir'%(PREFIX))
        info = LOCAL_SETTINGS % (locals())

        print 2,info

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_SETTINGS_H1)))
        cont += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_SETTINGS_P1)))
        cont += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_SETTINGS_P2)))
        cont += CTK.Notice (content=CTK.RawHTML('<pre>%s</pre>'%(info)))

        print 3

        cont += submit
        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class WebDirectory:
    def __call__ (self):
        table = CTK.PropsTable()
        table.Add (_('Web Directory'), CTK.TextCfg ('%s!web_dir'%(PREFIX), False, {'value': '/wiki', 'class': 'noauto'}), _(NOTE_WEBDIR))
        table.Add (_('Internal Web Directory'), CTK.TextCfg ('%s!aux_dir'%(PREFIX), False, {'value': '/w', 'class': 'noauto'}), _(NOTE_AUXDIR))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WEBDIR_H1)))
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevNext_Auto()
        return cont.Render().toStr()


class Host:
    def __call__ (self):
        table = CTK.PropsTable()
        table.Add (_('New Host Name'),    CTK.TextCfg ('%s!host'%(PREFIX), False, {'value': 'www.example.com', 'class': 'noauto'}), _(NOTE_HOST))
        table.Add (_('Use Same Logs as'), Wizard.CloneLogsCfg('%s!logs_as_vsrv'%(PREFIX)), _(Wizard.CloneLogsCfg.NOTE))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_HOST_H1)))
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevNext_Auto()
        return cont.Render().toStr()


class LocalSource:
    def __call__ (self):
        guessed_src = path_find_w_default (SRC_PATHS)

        table = CTK.PropsTable()
        table.Add (_('MediaWiki Local Directory'), CTK.TextCfg ('%s!local_dir'%(PREFIX), False, {'value': guessed_src}), _(NOTE_LOCAL_DIR))

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
        cont += Wizard.Icon ('mediawiki', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P2)))
        box += Wizard.CookBookBox ('http://www.mediawiki.org/wiki/Manual:Short_URL/wiki/Page_title_--_Cherokee_rewrite--root_access',
                                   _('This wizard is based on the Cherokee wiki-article at MediaWiki.org.'))
        cont += box

        # Send the VServer num if it's a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


def is_mediawiki_dir (path):
    path = validations.is_local_dir_exists (path)
    module_inc = os.path.join (path, 'includes/Article.php')
    try:
        validations.is_local_file_exists (module_inc)
    except:
        raise ValueError, _(ERROR_NO_SRC)
    return path


VALS = [
    ('%s!local_dir'%(PREFIX), validations.is_not_empty),
    ('%s!host'     %(PREFIX), validations.is_not_empty),
    ('%s!web_dir'  %(PREFIX), validations.is_not_empty),

    ('%s!local_dir'%(PREFIX), is_mediawiki_dir),
    ('%s!host'     %(PREFIX), validations.is_new_vserver_nick),
    ('%s!web_dir'  %(PREFIX), validations.is_dir_formatted)
]

# VServer
CTK.publish ('^/wizard/vserver/mediawiki$',   Welcome)
CTK.publish ('^/wizard/vserver/mediawiki/2$', PHP)
CTK.publish ('^/wizard/vserver/mediawiki/3$', LocalSource)
CTK.publish ('^/wizard/vserver/mediawiki/4$', WebDirectory)
CTK.publish ('^/wizard/vserver/mediawiki/5$', Host)
CTK.publish ('^/wizard/vserver/mediawiki/6$', ShowLocalSettings)

# Rule
CTK.publish ('^/wizard/vserver/(\d+)/mediawiki$',   Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/mediawiki/2$', PHP)
CTK.publish ('^/wizard/vserver/(\d+)/mediawiki/3$', LocalSource)
CTK.publish ('^/wizard/vserver/(\d+)/mediawiki/4$', WebDirectory)
CTK.publish ('^/wizard/vserver/(\d+)/mediawiki/5$', ShowLocalSettings)

# Common
CTK.publish (r'^%s$'%(URL_APPLY), Commit, method="POST", validation=VALS)
