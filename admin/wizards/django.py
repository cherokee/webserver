# -*- coding: utf-8 -*-
#
# Cherokee-admin's Django wizard
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
# 2010/04/13: Django 1.1.1 / Cherokee 0.99.41
#

import os
import re
import CTK
import Wizard
import validations
from util import *

NOTE_WELCOME_H1 = N_("Welcome to the Django wizard")
NOTE_WELCOME_P1 = N_('<a target="_blank" href="http://djangoproject.com/">Django</a> is a high-level Python Web framework that encourages rapid development and clean, pragmatic design.')
NOTE_WELCOME_P2 = N_('It is the Web framework for perfectionists (with deadlines). Django makes it easier to build better Web apps more quickly and with less code.')
NOTE_LOCAL_H1   = N_("Application Source Code")
NOTE_LOCAL_DIR  = N_("Local directory where your Django project source code is located.")
NOTE_HOST_H1    = N_("New Virtual Server Details")
NOTE_HOST       = N_("Host name of the virtual server that is about to be created.")
NOTE_WEBDIR     = N_("Web directory where you want Django to be accessible. (Example: /blog)")
NOTE_WEBDIR_H1  = N_("Public Web Directory")
NOTE_DROOT      = N_("Path to use as document root for the new virtual server.")
ERROR_NO_SRC    = N_("Does not look like a Django source directory.")
ERROR_NO_DROOT  = N_("The document root directory does not exist.")

PREFIX = 'tmp!wizard!django'

URL_APPLY = '/wizard/vserver/django/apply'

SOURCE = """
source!%(src_num)d!type = interpreter
source!%(src_num)d!nick = Django %(src_num)d
source!%(src_num)d!host = 127.0.0.1:%(src_port)d
source!%(src_num)d!interpreter = %(python_bin)s %(django_dir)s/manage.py runfcgi protocol=scgi host=127.0.0.1 port=%(src_port)d
"""

CONFIG_VSERVER = SOURCE + """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = %(document_root)s

%(vsrv_pre)s!rule!10!match = directory
%(vsrv_pre)s!rule!10!match!directory = %(media_web_dir)s
%(vsrv_pre)s!rule!10!handler = file
%(vsrv_pre)s!rule!10!expiration = time
%(vsrv_pre)s!rule!10!expiration!time = 7d

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!encoder!gzip = 1
%(vsrv_pre)s!rule!1!handler = scgi
%(vsrv_pre)s!rule!1!handler!error_handler = 1
%(vsrv_pre)s!rule!1!handler!check_file = 0
%(vsrv_pre)s!rule!1!handler!balancer = round_robin
%(vsrv_pre)s!rule!1!handler!balancer!source!1 = %(src_num)d
"""

CONFIG_DIR = SOURCE + """
%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = %(webdir)s
%(rule_pre)s!encoder!gzip = 1
%(rule_pre)s!handler = scgi
%(rule_pre)s!handler!error_handler = 1
%(rule_pre)s!handler!check_file = 0
%(rule_pre)s!handler!balancer = round_robin
%(rule_pre)s!handler!balancer!source!1 = %(src_num)d
"""

class Commit:
    def Commit_VServer (self):
        # Create the new Virtual Server
        vsrv_pre = CTK.cfg.get_next_entry_prefix('vserver')
        CTK.cfg['%s!nick'%(vsrv_pre)] = CTK.cfg.get_val('%s!host'%(PREFIX))
        Wizard.CloneLogsCfg_Apply ('%s!logs_as_vsrv'%(PREFIX), vsrv_pre)

        # Django
        new_host      = CTK.cfg.get_val('%s!new_host'   %(PREFIX))
        django_dir    = CTK.cfg.get_val('%s!django_dir' %(PREFIX))
        document_root = CTK.cfg.get_val('%s!document_root' %(PREFIX))

        # Locals
        src_num, pre  = cfg_source_get_next ()
        src_port      = cfg_source_find_free_port ()

        # Analize Django config file
        media_web_dir = django_figure_media_prefix (django_dir)

        # Python 2
        # (When Django supports python3, this should be updated. Django 1.5?)
        python_bin = path_find_binary (['python2', 'python', 'python*'], default="python2")

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

        # Django
        webdir = CTK.cfg.get_val('%s!webdir' %(PREFIX))
        django_dir = CTK.cfg.get_val('%s!django_dir' %(PREFIX))

        # Locals
        rule_pre = CTK.cfg.get_next_entry_prefix('%s!rule'%(vsrv_pre))
        src_port = cfg_source_find_free_port ()
        src_num, src_pre  = cfg_source_get_next ()

        # Python 2
        # (When Django supports python3, this should be updated. Django 1.5?)
        python_bin = path_find_binary (['python2', 'python', 'python*'], default="python2")

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
        table.Add (_('Web Directory'), CTK.TextCfg ('%s!webdir'%(PREFIX), False, {'value': '/project', 'class': 'noauto'}), _(NOTE_WEBDIR))

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
        table.Add (_('Django Local Directory'), CTK.TextCfg ('%s!django_dir'%(PREFIX), False), _(NOTE_LOCAL_DIR))

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
        cont += Wizard.Icon ('django', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P2)))
        box += Wizard.CookBookBox ('cookbook_django')
        cont += box

        # Send the VServer num if it's a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


def django_figure_media_prefix (local_django_dir):
    prefix   = '/media'
    fullpath = os.path.join (local_django_dir, "settings.py")

    # Red the file
    if os.path.exists (fullpath):
        content = open(fullpath, "r").read()
        tmp = re.findall (r"\s*ADMIN_MEDIA_PREFIX\s*=\s*['\"](.*)['\"]", content)
        if tmp:
            prefix = tmp[0]
            if prefix.startswith("http"):
                n = prefix.index("://")
                s = prefix.index("/", n+4)
                prefix = prefix[s:]

    # Remove trailing slash
    while len(prefix)>1 and prefix[-1] == '/':
        prefix = prefix[:-1]

    return prefix


def is_django_dir (path):
    path = validations.is_local_dir_exists (path)
    manage = os.path.join (path, "manage.py")

    try:
        validations.is_local_file_exists (manage)
    except:
        raise ValueError, _("Directory doesn't look like a Django based project.")
    return path


VALS = [
    ("%s!django_dir"   %(PREFIX), validations.is_not_empty),
    ("%s!new_host"     %(PREFIX), validations.is_not_empty),
    ("%s!webdir"       %(PREFIX), validations.is_not_empty),
    ("%s!document_root"%(PREFIX), validations.is_not_empty),

    ('%s!django_dir'   %(PREFIX), is_django_dir),
    ('%s!new_host'     %(PREFIX), validations.is_new_vserver_nick),
    ('%s!webdir'       %(PREFIX), validations.is_dir_formatted),
    ("%s!document_root"%(PREFIX), validations.is_local_dir_exists)
]

# VServer
CTK.publish ('^/wizard/vserver/django$',   Welcome)
CTK.publish ('^/wizard/vserver/django/2$', LocalSource)
CTK.publish ('^/wizard/vserver/django/3$', Host)

# Rule
CTK.publish ('^/wizard/vserver/(\d+)/django$',   Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/django/2$', LocalSource)
CTK.publish ('^/wizard/vserver/(\d+)/django/3$', WebDirectory)

# Common
CTK.publish (r'^%s'%(URL_APPLY), Commit, method="POST", validation=VALS)
