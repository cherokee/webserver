# -*- coding: utf-8 -*-
#
# Cherokee-admin's Mailman Wizard
#
# Authors:
#      Taher Shihadeh <taher@unixwars.com>
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
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
# 2010/04/15: Mailman 2.1.12 / Cherokee 0.99.41
#

import re
import CTK
import Wizard
import validations
from util import *

NOTE_WELCOME_H1     = N_("Welcome to the Mailman Wizard")
NOTE_WELCOME_P1     = N_('<a target="_blank" href="http://www.gnu.org/software/mailman/">Mailman</a> is free software for managing electronic mail discussion and e-newsletter lists.')
NOTE_WELCOME_P2     = N_('Mailman supports built-in archiving, automatic bounce processing, content filtering, digest delivery, spam filters, and more.')

NOTE_LOCAL_H1       = N_('Mailman Details')
NOTE_LOCAL_CGI_DIR  = N_("Local path to the Mailman CGI directory.")
NOTE_LOCAL_DATA_DIR = N_("Local path to the Mailman data directory.")
NOTE_LOCAL_ARCH_DIR = N_("Local path to the Mailman mail archive directory.")
NOTE_LOCAL_IMGS_DIR = N_("Local path to the Mailman mail images directory.")

NOTE_HOST_H1        = N_("New Virtual Server Details")
NOTE_HOST           = N_("Host name of the virtual server that is about to be created.")

PREFIX    = 'tmp!wizard!mailman'
URL_APPLY = r'/wizard/vserver/mailman/apply'

CONFIG_VSERVER = """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = /dev/null

%(vsrv_pre)s!rule!500!document_root = %(mailman_imgs_dir)s
%(vsrv_pre)s!rule!500!handler = file
%(vsrv_pre)s!rule!500!match = directory
%(vsrv_pre)s!rule!500!match!directory = /images/mailman

%(vsrv_pre)s!rule!400!match = directory
%(vsrv_pre)s!rule!400!match!directory = /pipermail
%(vsrv_pre)s!rule!400!match!final = 1
%(vsrv_pre)s!rule!400!document_root = %(mailman_arch_dir)s/archives/public
%(vsrv_pre)s!rule!400!encoder!gzip = 1
%(vsrv_pre)s!rule!400!handler = common
%(vsrv_pre)s!rule!400!handler!allow_dirlist = 1

%(vsrv_pre)s!rule!300!match = directory
%(vsrv_pre)s!rule!300!match!directory = /icons
%(vsrv_pre)s!rule!300!match!final = 1
%(vsrv_pre)s!rule!300!document_root = %(mailman_data_dir)s/icons
%(vsrv_pre)s!rule!300!handler = file
%(vsrv_pre)s!rule!300!handler!iocache = 1

%(vsrv_pre)s!rule!200!match = fullpath
%(vsrv_pre)s!rule!200!match!final = 1
%(vsrv_pre)s!rule!200!match!fullpath!1 = /
%(vsrv_pre)s!rule!200!encoder!gzip = 1
%(vsrv_pre)s!rule!200!handler = redir
%(vsrv_pre)s!rule!200!handler!rewrite!1!show = 1
%(vsrv_pre)s!rule!200!handler!rewrite!1!substring = /listinfo

%(vsrv_pre)s!rule!100!match = default
%(vsrv_pre)s!rule!100!match!final = 1
%(vsrv_pre)s!rule!100!document_root = %(mailman_cgi_dir)s
%(vsrv_pre)s!rule!100!encoder!gzip = 1
%(vsrv_pre)s!rule!100!handler = cgi
%(vsrv_pre)s!rule!100!handler!change_user = 1
%(vsrv_pre)s!rule!100!handler!check_file = 1
%(vsrv_pre)s!rule!100!handler!error_handler = 1
%(vsrv_pre)s!rule!100!handler!pass_req_headers = 1
%(vsrv_pre)s!rule!100!handler!xsendfile = 0
"""

SRC_PATHS_CGI = [
    "/usr/local/mailman/cgi-bin",
    "/usr/lib/cgi-bin/mailman",
    "/opt/local/libexec/mailman/cgi-bin"
]

SRC_PATHS_DATA = [
    "/usr/lib/mailman",
    "/usr/local/mailman",
    "/usr/share/mailman",
    "/opt/local/share/mailman"
]

SRC_PATHS_ARCH = [
    "/var/lib/mailman",
    "/usr/local/mailman",
    "/var/share/mailman",
    "/opt/local/var/mailman/"
]

SRC_PATHS_IMGS = [
    "/usr/share/images/mailman",
    "/opt/local/share/mailman/icons",
    "/opt/mailman*/admin/www/images"
]

class Commit:
    def Commit_VServer (self):
        # Incoming info
        mailman_cgi_dir  = CTK.cfg.get_val('%s!mailman_cgi_dir'%(PREFIX))
        mailman_data_dir = CTK.cfg.get_val('%s!mailman_data_dir'%(PREFIX))
        mailman_arch_dir = CTK.cfg.get_val('%s!mailman_arch_dir'%(PREFIX))
        mailman_imgs_dir = CTK.cfg.get_val('%s!mailman_imgs_dir'%(PREFIX))
        new_host         = CTK.cfg.get_val('%s!new_host'%(PREFIX))

        # Create the new Virtual Server
        vsrv_pre = CTK.cfg.get_next_entry_prefix('vserver')
        CTK.cfg['%s!nick'%(vsrv_pre)] = new_host
        Wizard.CloneLogsCfg_Apply ('%s!logs_as_vsrv'%(PREFIX), vsrv_pre)

        # Add the new rules
        config = CONFIG_VSERVER %(locals())
        CTK.cfg.apply_chunk (config)

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(vsrv_pre))
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
        guessed_cgi  = path_find_w_default (SRC_PATHS_CGI)
        guessed_data = path_find_w_default (SRC_PATHS_DATA)
        guessed_arch = path_find_w_default (SRC_PATHS_ARCH)
        guessed_imgs = path_find_w_default (SRC_PATHS_IMGS)

        table = CTK.PropsTable()
        table.Add (_('Mailman CGI directory'),  CTK.TextCfg ('%s!mailman_cgi_dir'%(PREFIX),  False, {'value':guessed_cgi}),  _(NOTE_LOCAL_CGI_DIR))
        table.Add (_('Mailman Data directory'), CTK.TextCfg ('%s!mailman_data_dir'%(PREFIX), False, {'value':guessed_data}), _(NOTE_LOCAL_DATA_DIR))
        table.Add (_('Mail Archive directory'), CTK.TextCfg ('%s!mailman_arch_dir'%(PREFIX), False, {'value':guessed_arch}), _(NOTE_LOCAL_ARCH_DIR))
        table.Add (_('Mail Images directory'),  CTK.TextCfg ('%s!mailman_imgs_dir'%(PREFIX), False, {'value':guessed_imgs}), _(NOTE_LOCAL_IMGS_DIR))

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
        cont += Wizard.Icon ('mailman', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P2)))
        box += Wizard.CookBookBox ('cookbook_mailman')
        cont += box

        # Send the VServer num if it is a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


def is_mailman_data_dir (path):
    path = validations.is_local_dir_exists (path)
    file = os.path.join (path, "bin/newlist")
    if not os.path.exists (file):
        raise ValueError, _("It does not look like a Mailman data directory.")
    return path

def is_mailman_cgi_dir (path):
    path = validations.is_local_dir_exists (path)
    file = os.path.join (path, "listinfo")
    if not os.path.exists (file):
        raise ValueError, _("It does not look like a Mailman CGI directory.")
    return path

def is_mailman_arch_dir (path):
    path = validations.is_local_dir_exists (path)
    file = os.path.join (path, "archives/public")
    try:
        validations.is_local_dir_exists (file)
    except:
        raise ValueError, _("It does not look like a Mailman archive directory.")
    return path

def is_mailman_imgs_dir (path):
    path = validations.is_local_dir_exists (path)
    file = os.path.join (path, "mailman.jpg")
    if not os.path.exists (file):
        raise ValueError, _("It does not look like a Mailman images directory.")
    return path


VALS = [
    ('%s!new_host'        %(PREFIX), validations.is_not_empty),
    ('%s!mailman_cgi_dir' %(PREFIX), validations.is_not_empty),
    ('%s!mailman_data_dir'%(PREFIX), validations.is_not_empty),
    ('%s!mailman_arch_dir'%(PREFIX), validations.is_not_empty),
    ('%s!mailman_imgs_dir'%(PREFIX), validations.is_not_empty),

    ("%s!new_host"        %(PREFIX), validations.is_new_vserver_nick),
    ("%s!mailman_cgi_dir" %(PREFIX), is_mailman_cgi_dir),
    ("%s!mailman_data_dir"%(PREFIX), is_mailman_data_dir),
    ("%s!mailman_arch_dir"%(PREFIX), is_mailman_arch_dir),
    ("%s!mailman_imgs_dir"%(PREFIX), is_mailman_imgs_dir),
]

# VServer
CTK.publish ('^/wizard/vserver/mailman$',   Welcome)
CTK.publish ('^/wizard/vserver/mailman/2$', LocalSource)
CTK.publish ('^/wizard/vserver/mailman/3$', Host)
CTK.publish (r'^%s'%(URL_APPLY), Commit, method="POST", validation=VALS)
