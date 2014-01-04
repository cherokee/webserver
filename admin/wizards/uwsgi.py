# -*- coding: utf-8 -*-
#
# Cherokee-admin's uWSGI Wizard
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
# 2009/10/xx: uWSGI Version 0.9.3   / Cherokee 0.99.36b
# 2010/04/15: uWSGI Version 0.9.3   / Cherokee 0.99.41
# 2010/04/22: uWSGI Version 0.9.4.3 / Cherokee 0.99.45b
# 2010/12/01: uWSGI Version 0.9.6.5 + 0.9.7-dev / Cherokee SVN: r5847

import os
import sys
import socket
import re
import CTK
import Wizard
import validations
from xml.dom import minidom
import ConfigParser

try:
    import yaml
    UWSGI_HAS_YAML=True
except:
    UWSGI_HAS_YAML=False

from util import *

NOTE_WELCOME_H1   = N_("Welcome to the uWSGI Wizard")
NOTE_WELCOME_P1   = N_('<a target="_blank" href="http://projects.unbit.it/uwsgi/">uWSGI</a> is a fast (pure C), self-healing, developer-friendly application container, aimed for professional webapps deployment and development.')
NOTE_WELCOME_P2   = N_('It includes a complete stack for networked/clustered applications, implementing message/object passing and process management. It uses the uwsgi (all lowercase) protocol for all the networking/interprocess communications.')

NOTE_LOCAL_H1     = N_("uWSGI")
NOTE_UWSGI_CONFIG = N_("Path to the uWSGI configuration file (XML, INI, YAML or .wsgi, .py, .psgi, .pl, .lua, .ws, .ru, .rb). Its mountpoint will be used.")
NOTE_UWSGI_BINARY = N_("Location of the uWSGI binary")
ERROR_NO_CONFIG   = N_("It does not look like a uWSGI configuration file.")

NOTE_HOST_H1      = N_("New Virtual Server Details")
NOTE_HOST         = N_("Host name of the virtual server that is about to be created.")
NOTE_DROOT        = N_("Path to use as document root for the new virtual server.")

NOTE_WEBDIR       = N_("Public web directory to access the project.")
NOTE_WEBDIR_H1    = N_("Public Web Directory")
NOTE_WEBDIR_P1    = N_("The default value is extracted from the configuration file. Change it at your own risk.")

PREFIX    = 'tmp!wizard!uwsgi'
URL_APPLY = r'/wizard/vserver/uwsgi/apply'

UWSGI_CMDLINE_AUTOMAGIC = "-M -p %(CPU_num)d -z %(timeout)s -L -l %(SOMAXCONN)d %(filename)s"
UWSGI_DEFAULT_CONFS = ('.xml', '.ini', '.yml',)
UWSGI_MAGIC_CONFS = ('.wsgi', '.py', '.psgi', '.pl', '.lua', '.ws', '.ru', '.rb',)

SOURCE = """
source!%(src_num)d!env_inherited = 1
source!%(src_num)d!type = interpreter
source!%(src_num)d!nick = uWSGI %(src_num)d
source!%(src_num)d!host = %(src_addr)s
source!%(src_num)d!interpreter = %(uwsgi_binary)s %(uwsgi_extra)s
"""

SINGLE_DIRECTORY = """
%(vsrv_pre)s!rule!%(rule_id)d!match = directory
%(vsrv_pre)s!rule!%(rule_id)d!match!directory = %(webdir)s
%(vsrv_pre)s!rule!%(rule_id)d!handler = uwsgi
%(vsrv_pre)s!rule!%(rule_id)d!handler!error_handler = 1
%(vsrv_pre)s!rule!%(rule_id)d!handler!check_file = 0
%(vsrv_pre)s!rule!%(rule_id)d!handler!pass_req_headers = 1
%(vsrv_pre)s!rule!%(rule_id)d!handler!balancer = round_robin
%(vsrv_pre)s!rule!%(rule_id)d!handler!modifier1 = %(modifier1)d
%(vsrv_pre)s!rule!%(rule_id)d!handler!modifier2 = 0
%(vsrv_pre)s!rule!%(rule_id)d!handler!balancer!source!1 = %(src_num)d

"""

CONFIG_VSERVER = SOURCE + """
%(vsrv_pre)s!nick = %(new_host)s
%(vsrv_pre)s!document_root = %(document_root)s

"""

DEFAULT_DIRECTORY = """
%(vsrv_pre)s!rule!1!match = default
%(vsrv_pre)s!rule!1!handler = common
%(vsrv_pre)s!rule!1!handler!iocache = 0
"""

CONFIG_DIR = SOURCE + """
%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = %(webdir)s
%(rule_pre)s!handler = uwsgi
%(rule_pre)s!handler!error_handler = 1
%(rule_pre)s!handler!check_file = 0
%(rule_pre)s!handler!pass_req_headers = 1
%(rule_pre)s!handler!balancer = round_robin
%(rule_pre)s!handler!modifier1 = %(modifier1)d
%(rule_pre)s!handler!modifier2 = 0
%(rule_pre)s!handler!balancer!source!1 = %(src_num)d
"""

DEFAULT_BINS  = ['uwsgi','uwsgi26','uwsgi25']

DEFAULT_PATHS = ['/usr/bin',
                 '/usr/sbin',
                 '/usr/local/bin',
                 '/usr/local/sbin',
                 '/usr/gnu/bin',
                 '/opt/local/sbin',
                 '/opt/local/bin']


def figure_CPU_num():
    if 'SC_NPROCESSORS_ONLN'in os.sysconf_names:
        return os.sysconf('SC_NPROCESSORS_ONLN')

    proc = subprocess.Popen("sysctl -n hw.ncpu", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout_results, stderr_results = proc.communicate()
    if len(stderr_results) == 0:
        return int(stdout_results)

    return 1

class Commit:
    def Commit_VServer (self):
        # Incoming info
        document_root = CTK.cfg.get_val('%s!document_root'%(PREFIX))
        uwsgi_cfg     = CTK.cfg.get_val('%s!uwsgi_cfg'%(PREFIX))
        new_host      = CTK.cfg.get_val('%s!new_host'%(PREFIX))

        # Create the new Virtual Server
        vsrv_pre = CTK.cfg.get_next_entry_prefix('vserver')
        CTK.cfg['%s!nick'%(vsrv_pre)] = new_host
        Wizard.CloneLogsCfg_Apply ('%s!logs_as_vsrv'%(PREFIX), vsrv_pre)

        uwsgi_binary = find_uwsgi_binary()
        if not uwsgi_binary:
            uwsgi_binary = CTK.cfg.get_val('%s!uwsgi_binary' %(PREFIX))

        # Locals
        src_num, pre = cfg_source_get_next ()

        uwsgi_extra = uwsgi_get_extra(uwsgi_cfg)
        modifier1 = uwsgi_get_modifier(uwsgi_cfg)
        src_addr = uwsgi_get_socket(uwsgi_cfg)
        if not src_addr:
            src_addr = "127.0.0.1:%d" % cfg_source_find_free_port ()
            uwsgi_extra = "-s %s %s" % (src_addr, uwsgi_extra)
        elif src_addr.startswith(':'):
            src_addr = "127.0.0.1%s" % src_addr

        # Build the config
        cvs = CONFIG_VSERVER
        webdirs = uwsgi_find_mountpoint (uwsgi_cfg)
        rule_id = 2
        for webdir in webdirs:
            cvs += SINGLE_DIRECTORY %(locals())
            rule_id = rule_id + 1

        cvs += DEFAULT_DIRECTORY

        # Add the new rules
        config = cvs %(locals())

        CTK.cfg.apply_chunk (str(config))

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(vsrv_pre))
        CTK.cfg.normalize ('vserver')

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()

    def Commit_Rule (self):
        vsrv_num = CTK.cfg.get_val('%s!vsrv_num' %(PREFIX))
        vsrv_pre = 'vserver!%s' %(vsrv_num)

        # Incoming info
        uwsgi_cfg = CTK.cfg.get_val('%s!uwsgi_cfg' %(PREFIX))

        uwsgi_binary = find_uwsgi_binary()
        if not uwsgi_binary:
            uwsgi_binary = CTK.cfg.get_val('%s!uwsgi_binary' %(PREFIX))

        # Locals
        rule_pre = CTK.cfg.get_next_entry_prefix ('%s!rule' %(vsrv_pre))
        src_num, src_pre = cfg_source_get_next ()

        uwsgi_extra = uwsgi_get_extra(uwsgi_cfg)
        modifier1 = uwsgi_get_modifier(uwsgi_cfg)
        src_addr = uwsgi_get_socket(uwsgi_cfg)
        if not src_addr:
            src_addr = "127.0.0.1:%d" % cfg_source_find_free_port ()
            uwsgi_extra = "-s %s %s" % (src_addr, uwsgi_extra)
        else:
            if src_addr.startswith(':'):
                src_addr = "127.0.0.1%s" % src_addr

        # Add the new rules
        webdir = uwsgi_find_mountpoint(uwsgi_cfg)[0]
        config = CONFIG_DIR %(locals())
        CTK.cfg.apply_chunk (str(config))

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(vsrv_pre))

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()

    def __call__ (self):
        if CTK.post.pop('final'):
            # Apply POST
            CTK.cfg_apply_post()

            # VServer or Rule?
            if CTK.cfg.get_val ('%s!vsrv_num' %(PREFIX)):
                return self.Commit_Rule()
            return self.Commit_VServer()

        return CTK.cfg_apply_post()


class WebDirectory:
    def __call__ (self):
        uwsgi_cfg = CTK.cfg.get_val('%s!uwsgi_cfg' %(PREFIX))
        webdir    = uwsgi_find_mountpoint(uwsgi_cfg)[0]

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
        table.Add (_('New Host Name'),    CTK.TextCfg ('%s!new_host'%(PREFIX),      False, {'value': 'www.example.com', 'class': 'noauto'}), _(NOTE_HOST))
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
        uwsgi_binary = find_uwsgi_binary()

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
        box  = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P2)))
        box += Wizard.CookBookBox ('cookbook_uwsgi')

        cont  = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('uwsgi', {'class': 'wizard-descr'})
        cont += box

        # Send the VServer num if it is a Rule
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        if tmp:
            submit  = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


#
# Utility functions
#

def is_uwsgi_cfg (filename):
    filename   = validations.is_local_file_exists (filename)

    for k in UWSGI_DEFAULT_CONFS:
        if filename.endswith(k):
            return filename

    for k in UWSGI_MAGIC_CONFS:
        if filename.endswith(k):
            return filename
    return filename

def uwsgi_get_extra(filename):
    if filename.endswith('.xml'):
        return "-x %s" % filename
    elif filename.endswith('.ini'):
        return "--ini %s" % filename
    elif filename.endswith('.yml'):
        return "--yaml %s" % filename

    CPU_num   = figure_CPU_num() * 2
    timeout   = CTK.cfg.get_val('server!timeout', '15')
    SOMAXCONN = socket.SOMAXCONN

    return UWSGI_CMDLINE_AUTOMAGIC %(locals())


def uwsgi_get_modifier(filename):
    if filename.endswith('.psgi') or filename.endswith('.pl'):
        return 5
    if filename.endswith('.lua') or filename.endswith('.ws'):
        return 6
    if filename.endswith('.ru') or filename.endswith('.rb'):
        return 7
    return 0

def uwsgi_get_socket(filename):
    s = None
    if filename.endswith('.xml'):
        try:
            udom = minidom.parse(filename)
            uroot = udom.getElementsByTagName('uwsgi')[0]
            s = uroot.getElementsByTagName('socket')[0].childNodes[0].data
        except:
            pass

    elif filename.endswith('.ini'):
        try:
            c = ConfigParser.ConfigParser()
            c.read(filename)
            s = c.get('uwsgi', 'socket')
        except:
            pass

    elif filename.endswith('.yml') and UWSGI_HAS_YAML:
        try:
            fd = open(filename, 'r')
            y = yaml.load(fd)
            s = y['uwsgi']['socket']
            fd.close()
        except:
            pass

    return s

def uwsgi_find_mountpoint (filename):
    mp       = []
    found_mp = False

    if filename.endswith('.xml'):
        try:
            udom = minidom.parse(filename)
            uroot = udom.getElementsByTagName('uwsgi')[0]
            for m in uroot.getElementsByTagName('app'):
                try:
                    path = CTK.util.to_utf8 (m.attributes['mountpoint'].value)
                    mp.append (path)
                    found_mp = True
                except:
                    pass
        except:
            pass

    if found_mp:
        return mp

    return ['/']

def find_uwsgi_binary():
    return path_find_binary (DEFAULT_BINS, extra_dirs = DEFAULT_PATHS)


#
# Data Validation
#

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
CTK.publish (r'^%s'%(URL_APPLY), Commit, method="POST", validation=VALS)
