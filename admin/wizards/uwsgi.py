# -*- coding: utf-8 -*-
#
# Cherokee-admin's uWSGI Wizard
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
# 2009/10/xx: uWSGI Version 0.9.3   / Cherokee 0.99.36b
# 2010/04/15: uWSGI Version 0.9.3   / Cherokee 0.99.41
# 2010/04/22: uWSGI Version 0.9.4.3 / Cherokee 0.99.45b

import re
import CTK
import Wizard
import validations
from util import *

NOTE_WELCOME_H1   = N_("Welcome to the uWsgi Wizard")
NOTE_WELCOME_P1   = N_('<a target="_blank" href="http://projects.unbit.it/uwsgi/">uWSGI</a> is a fast (pure C), self-healing, developer-friendly WSGI server, aimed for professional python webapps deployment and development.')
NOTE_WELCOME_P2   = N_('Over time it has evolved in a complete stack for networked/clustered python applications, implementing message/object passing and process management. It uses the uwsgi (all lowercase) protocol for all the networking/interprocess communications.')

NOTE_LOCAL_H1     = N_("uWSGI")
NOTE_UWSGI_CONFIG = N_("Path to the uWSGI configuration file (XML or Python-only). Its mountpoint will be used.")
NOTE_UWSGI_BINARY = N_("Location of the uWSGI binary")
ERROR_NO_CONFIG   = N_("It does not look like a uWSGI configuration file.")

NOTE_HOST_H1    = N_("New Virtual Server Details")
NOTE_HOST       = N_("Host name of the virtual server that is about to be created.")
NOTE_DROOT      = N_("Path to use as document root for the new virtual server.")

NOTE_WEBDIR     = N_("Public web directory to access the project.")
NOTE_WEBDIR_H1  = N_("Public Web Direcoty")
NOTE_WEBDIR_P1  = N_("The default value is extracted from the configuration file. Change it at your own risk.")
ERROR_NO_SRC    = N_("Does not look like a Uwsgi source directory.")
ERROR_NO_DROOT  = N_("The document root directory does not exist.")

PREFIX    = 'tmp!wizard!uwsgi'

URL_APPLY      = r'/wizard/vserver/uwsgi/apply'

SOURCE = """
source!%(src_num)d!type = interpreter
source!%(src_num)d!nick = uWSGI %(src_num)d
source!%(src_num)d!host = 127.0.0.1:%(src_port)d
source!%(src_num)d!interpreter = %(uwsgi_binary)s -s 127.0.0.1:%(src_port)d -t 10 -M -p 1 -C %(uwsgi_extra)s
"""

SOURCE_XML   = """ -x %s"""
SOURCE_WSGI  = """ -w %s"""
SOURCE_VE    = """ -H %s"""

CONFIG_VSERVER = SOURCE + """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = %(document_root)s

%(vsrv_pre)s!rule!2!match = directory
%(vsrv_pre)s!rule!2!match!directory = %(webdir)s
%(vsrv_pre)s!rule!2!encoder!gzip = 1
%(vsrv_pre)s!rule!2!handler = uwsgi
%(vsrv_pre)s!rule!2!handler!error_handler = 1
%(vsrv_pre)s!rule!2!handler!check_file = 0
%(vsrv_pre)s!rule!2!handler!pass_req_headers = 1
%(vsrv_pre)s!rule!2!handler!balancer = round_robin
%(vsrv_pre)s!rule!2!handler!balancer!source!1 = %(src_num)d

%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!handler = common
%(vsrv_pre)s!rule!1!handler!iocache = 0
"""

CONFIG_DIR = SOURCE + """
%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = %(webdir)s
%(rule_pre)s!encoder!gzip = 1
%(rule_pre)s!handler = uwsgi
%(rule_pre)s!handler!error_handler = 1
%(rule_pre)s!handler!check_file = 0
%(rule_pre)s!handler!pass_req_headers = 1
%(rule_pre)s!handler!balancer = round_robin
%(rule_pre)s!handler!balancer!source!1 = %(src_num)d
"""

DEFAULT_BINS   = ['uwsgi','uwsgi26','uwsgi25']

DEFAULT_PATHS  = ['/usr/bin',
                  '/usr/gnu/bin',
                  '/opt/local/bin']

RE_MOUNTPOINT_XML = """<uwsgi>.*?<app mountpoint=["|'](.*?)["|']>.*</uwsgi>"""
RE_MOUNTPOINT_WSGI  = """applications.*=.*{.*'(.*?)':"""


class Commit:
    def Commit_VServer (self):
        # Incoming info
        document_root = CTK.cfg.get_val('%s!document_root'%(PREFIX))
        uwsgi_cfg     = CTK.cfg.get_val('%s!uwsgi_cfg'%(PREFIX))
        new_host      = CTK.cfg.get_val('%s!new_host'%(PREFIX))
        webdir        = find_mountpoint_xml(uwsgi_cfg)

        # Create the new Virtual Server
        vsrv_pre = CTK.cfg.get_next_entry_prefix('vserver')
        CTK.cfg['%s!nick'%(vsrv_pre)] = new_host
        Wizard.CloneLogsCfg_Apply ('%s!logs_as_vsrv'%(PREFIX), vsrv_pre)

        # Choose between XML config+launcher, or Python only config
        if webdir:
            uwsgi_extra = SOURCE_XML % (uwsgi_cfg)
        else:
            webdir   = find_mountpoint_wsgi(uwsgi_cfg)
            par_name = os.path.dirname(os.path.normpath (uwsgi_cfg)).split(os.path.sep)[-1]
            cfg_name = os.path.basename(os.path.normpath(uwsgi_cfg))
            module   = '%s.%s' %(par_name, os.path.splitext(cfg_name)[0])
            uwsgi_extra = SOURCE_WSGI % (module)

        # Virtualenv support
        uwsgi_virtualenv = find_virtualenv(uwsgi_cfg)
        if uwsgi_virtualenv:
            uwsgi_extra += SOURCE_VE % uwsgi_virtualenv

        uwsgi_binary  = find_uwsgi_binary()
        if not uwsgi_binary:
            uwsgi_binary = CTK.cfg.get_val('%s!uwsgi_binary'%(PREFIX))

        # Locals
        src_num, pre  = cfg_source_get_next ()
        src_port      = cfg_source_find_free_port ()

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
        uwsgi_cfg     = CTK.cfg.get_val('%s!uwsgi_cfg'%(PREFIX))
        webdir        = find_mountpoint_xml(uwsgi_cfg)

        # Choose between XML config+launcher, or Python only config
        if webdir:
            uwsgi_extra = SOURCE_XML % uwsgi_cfg
        else:
            webdir = find_mountpoint_wsgi(uwsgi_cfg)
            uwsgi_extra = SOURCE_WSGI % uwsgi_cfg

        # Virtualenv support
        uwsgi_virtualenv = find_virtualenv(uwsgi_cfg)
        if uwsgi_virtualenv:
            uwsgi_extra += SOURCE_VE % uwsgi_virtualenv

        uwsgi_binary  = find_uwsgi_binary
        if not uwsgi_binary:
            uwsgi_binary = CTK.cfg.get_val('%s!uwsgi_binary'%(PREFIX))

        # Locals
        rule_pre = CTK.cfg.get_next_entry_prefix('%s!rule'%(vsrv_pre))
        src_port = cfg_source_find_free_port ()
        src_num, src_pre  = cfg_source_get_next ()

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
        uwsgi_cfg  = CTK.cfg.get_val('%s!uwsgi_cfg'%(PREFIX))
        webdir     = find_mountpoint_xml(uwsgi_cfg)
        if not webdir:
            webdir = find_mountpoint_wsgi(uwsgi_cfg)

        table = CTK.PropsTable()
        table.Add (_('Web Directory'), CTK.TextCfg ('%s!webdir'%(PREFIX), False, {'value': webdir, 'class': 'noauto'}), _(NOTE_WEBDIR))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WEBDIR_H1)))
        cont += submit
        cont += CTK.Notice('warning', CTK.RawHTML(_(NOTE_WEBDIR_P1)))
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
        uwsgi_binary = find_uwsgi_binary ()

        table = CTK.PropsTable()
        if not uwsgi_binary:
            table.Add (_('uWSGI binary'),   CTK.TextCfg ('%s!uwsgi_binary'%(PREFIX), False), _(NOTE_UWSGI_BINARY))
        table.Add (_('Configuration File'), CTK.TextCfg ('%s!uwsgi_cfg'   %(PREFIX), False), _(NOTE_UWSGI_CONFIG))

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
        cont += Wizard.Icon ('uwsgi', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P2)))
        cont += box

        # Send the VServer num if it is a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


def is_uwsgi_cfg (filename):
    filename = validations.is_local_file_exists (filename)
    mountpoint = find_mountpoint_xml(filename)
    if not mountpoint:
        mountpoint = find_mountpoint_wsgi(filename)
        if not mountpoint:
            raise ValueError, _(ERROR_NO_CONFIG)
    return filename

def find_virtualenv (filename):
    try:
        import virtualenv
    except ImportError:
        return None

    dirname = os.path.dirname(os.path.normpath(filename))
    return os.path.normpath(dirname + os.path.sep + os.path.pardir)

def find_mountpoint_xml (filename):
    regex = re.compile(RE_MOUNTPOINT_XML, re.DOTALL)
    fullname = get_real_path (filename)
    match = regex.search (open(fullname).read())
    if match:
        return match.groups()[0]

def find_mountpoint_wsgi (filename):
    regex = re.compile(RE_MOUNTPOINT_WSGI, re.DOTALL)
    fullname = get_real_path (filename)
    match = regex.search (open(fullname).read())
    if match:
        return match.groups()[0]

def find_uwsgi_binary ():
    return path_find_binary (DEFAULT_BINS, extra_dirs = DEFAULT_PATHS)

VALS = [
    ("%s!uwsgi_binary" %(PREFIX), validations.is_not_empty),
    ("%s!uwsgi_cfg"    %(PREFIX), validations.is_not_empty),
    ("%s!new_host"     %(PREFIX), validations.is_not_empty),
    ("%s!document_root"%(PREFIX), validations.is_not_empty),
    ("%s!new_webdir"   %(PREFIX), validations.is_not_empty),

    ("%s!uwsgi_binary" %(PREFIX), validations.is_local_file_exists),
    ("%s!uwsgi_cfg"    %(PREFIX), is_uwsgi_cfg),
    ("%s!new_host"     %(PREFIX), validations.is_new_vserver_nick),
    ("%s!document_root"%(PREFIX), validations.is_local_dir_exists),
    ("%s!new_webdir"   %(PREFIX), validations.is_dir_formatted),
]

# VServer
CTK.publish ('^/wizard/vserver/uwsgi$',   Welcome)
CTK.publish ('^/wizard/vserver/uwsgi/2$', LocalSource)
CTK.publish ('^/wizard/vserver/uwsgi/3$', Host)

# Rule
CTK.publish ('^/wizard/vserver/(\d+)/uwsgi$',   Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/uwsgi/2$', LocalSource)
CTK.publish ('^/wizard/vserver/(\d+)/uwsgi/3$', WebDirectory)

# Common
CTK.publish (r'^%s$'%(URL_APPLY), Commit, method="POST", validation=VALS)
