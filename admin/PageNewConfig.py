# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#      Taher Shihadeh <taher@unixwars.com>
#
# Copyright (C) 2001-2010 Alvaro Lopez Ortega
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

import CTK
import Page
import Cherokee

import os
from config_version import config_version_get_current
from consts import *
from configured import *

URL_BASE  = '/create_config'
URL_APPLY = '/create_config/apply'

HELPS = [('index', N_("Index"))]

WARNING_NOT_FOUND = N_("<b>The configuration is not found</b>.<br />You can create a new configuration file and proceed to customize the web server.")

DEFAULT_PID_LOCATIONS = [
    '/var/run/cherokee.pid',
    os.path.join (PREFIX, 'var/run/cherokee.pid')
]

CHEROKEE_MIN_DEFAULT_CONFIG = """# Default configuration
server!pid_file = %s
vserver!1!nick = default
vserver!1!document_root = /tmp
vserver!1!rule!1!match = default
vserver!1!rule!1!handler = common
""" % (DEFAULT_PID_LOCATIONS[0])


class ConfigCreator:
    def __call__ (self, profile):
        if profile == 'regular':
            return self._create_config ("cherokee.conf.sample")

        elif profile == 'static':
            return self._create_config ("performance.conf.sample")

        elif profile  == 'development':
            if self._create_config ("cherokee.conf.sample"):
                self._tweak_config_for_dev()
            return True


    def _create_config (self, template_file):
        file = CTK.cfg.file
        if os.path.exists (file):
            return True

        dirname = os.path.dirname(file)
        if not os.path.exists (dirname):
            try:
                os.mkdir (dirname)
            except:
                return False

        content = "config!version = %s\n" %(config_version_get_current())

        conf_sample = os.path.join(CHEROKEE_ADMINDIR, template_file)
        if os.path.exists (conf_sample):
            content += open(conf_sample, 'r').read()
        else:
            content += CHEROKEE_MIN_DEFAULT_CONFIG

        try:
            f = open(file, 'w+')
            f.write (content)
            f.close()
        except:
            return False

        CTK.cfg.load()
        return True


    def _tweak_config_for_dev (self):
        del(CTK.cfg['server!bind'])
        del(CTK.cfg['server!pid_file'])
        del(CTK.cfg['vserver!1!logger'])

        CTK.cfg['server!bind!1!port']            = "1234"
        CTK.cfg['vserver!1!rule!5!handler!type'] = "normal"
        CTK.cfg['vserver!1!error_writer!type']   = "stderr"

        CTK.cfg['source!2!type'] = "host"
        CTK.cfg['source!2!nick'] = "localhost 8000"
        CTK.cfg['source!2!host'] = "localhost:8000"

        CTK.cfg.save()


def apply():
    creator = ConfigCreator()
    profile = CTK.post.pop('create')

    if creator (profile):
        return {'ret': 'ok', 'redirect': '/'}

    return {'ret': 'fail'}


class Form (CTK.Container):
    def __init__ (self, key, name, label, **kwargs):
        CTK.Container.__init__ (self, **kwargs)

        table = CTK.Table({'class':key})

        table.set_header(1)
        table += [CTK.RawHTML(name)]
        table += [CTK.RawHTML(label)]

        submit  = CTK.Submitter(URL_APPLY)
        submit += table
        submit += CTK.Hidden('create', key)
        submit += CTK.SubmitterButton (_('Create'))
        self += CTK.Indenter (submit)


class Render:
    def __call__ (self):
        container  = CTK.Container()
        container += CTK.RawHTML("<h2>%s</h2>" %(_('Create a new configuration file')))

        key        = 'regular'
        name       = _('Regular')
        label      = _('Regular configuration: Apache logs, MIME types, icons, etc.')
        container += Form (key, name, label)

        key        = 'static'
        name       = _('Static Content')
        label      = _('Optimized to send static content.')
        container += Form (key, name, label)

        key        = 'development'
        name       = _('Server Development')
        label      = _('No standard port, No log files, No PID file, etc.')
        container += Form (key, name, label)

        page = Page.Base(_('New Configuration File'), body_id='new_config', helps=HELPS)
        page += CTK.RawHTML("<h1>%s</h1>" %(_('Configuration File Not Found')))
        page += CTK.Notice ('warning', CTK.RawHTML(_(WARNING_NOT_FOUND)))
        page += CTK.Indenter (container)
        return page.Render()


CTK.publish ('^%s'%(URL_BASE), Render)
CTK.publish ('^%s'%(URL_APPLY), apply, method="POST")
