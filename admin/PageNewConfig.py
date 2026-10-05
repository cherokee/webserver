# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
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

import CTK
import Page
from CTK.i18n import _, N_
import Cherokee

import os
from config_version import config_version_get_current
from consts import *
from configured import *

URL_BASE  = '/create_config'
URL_APPLY = '/create_config/apply'

HELPS = [('index', N_("Index"))]

NOTE_LOADING        = N_("Loading new configuration file..")
WARNING_NOT_FOUND_1 = N_("The configuration file %(conf_file)s was not found.")
WARNING_NOT_FOUND_2 = N_("You can create a new configuration file and proceed to customize the web server.")

DEFAULT_PID_LOCATIONS = [
    '/var/run/cherokee.pid',
    os.path.join (PREFIX, 'var/run/cherokee.pid')
]

CHEROKEE_MIN_DEFAULT_CONFIG = f"""# Default configuration
server!pid_file = {DEFAULT_PID_LOCATIONS[0]}
vserver!1!nick = default
vserver!1!document_root = /tmp
vserver!1!rule!1!match = default
vserver!1!rule!1!handler = common
"""


class ConfigCreator:
    def __call__ (self, profile):
        if profile == 'regular':
            return self._create_config ("cherokee.conf.sample")

        elif profile == 'static':
            return self._create_config ("performance.conf.sample")

        elif profile  == 'development':
            re = self._create_config ("cherokee.conf.sample")
            if not re:
                return False

            self._tweak_config_for_dev()
            return True


    def _create_config (self, template_file):
        # Configuration file
        filename = CTK.cfg.file
        if filename is None:
            raise RuntimeError('CTK configuration file is not set')
        if os.path.exists (filename):
            return True

        dirname = os.path.dirname(filename)
        if dirname and not os.path.exists (dirname):
            try:
                os.mkdir (dirname)
            except:
                print(f"ERROR: Could not create directory '{dirname}'")
                return False

        # Configuration content
        content = f"config!version = {config_version_get_current()}\n"

        # Add basic content
        conf_sample_sys = os.path.join (CHEROKEE_ADMINDIR, template_file)
        conf_sample_dev = os.path.join (os.path.realpath (__file__ + f'/../../{template_file}'))

        if os.path.exists (conf_sample_sys):
            content += open(conf_sample_sys, 'r').read()
        elif os.path.exists (conf_sample_dev):
            content += open(conf_sample_dev, 'r').read()
        else:
            content += CHEROKEE_MIN_DEFAULT_CONFIG

        # Write it
        try:
            f = open(filename, 'w+')
            f.write (content)
            f.close()
        except:
            print(f"ERROR: Could not open '{filename}' for writing")
            return False

        CTK.cfg.load()
        return True


    def _tweak_config_for_dev (self):
        del(CTK.cfg['server!bind'])
        del(CTK.cfg['server!pid_file'])
        del(CTK.cfg['vserver!1!logger'])

        CTK.cfg['server!bind!1!port']            = "1234"
        CTK.cfg['server!log_flush_lapse']        = "0"
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
        return CTK.cfg_reply_ajax_ok()

    return {'ret': 'fail'}


class Form (CTK.Container):
    def __init__ (self, key, name, label, **kwargs):
        CTK.Container.__init__ (self, **kwargs)

        box = CTK.Box({'class': f'create-box {key}'})

        box += CTK.RawHTML(f'<h3>{name}</h3>')
        box += CTK.RawHTML(f'<span>{label}</span>')

        submit  = CTK.Submitter(URL_APPLY)
        submit += CTK.Hidden('create', key)
        submit += CTK.SubmitterButton (_('Create'))
        submit.bind ('submit_success',
                     f"$('#main').html('<h1>{NOTE_LOADING}</h1>');" + CTK.JS.GotoURL('/'))

        box += submit
        box += CTK.RawHTML('<div class="ui-helper-clearfix"></div>')

        self += box


class Render:
    def __call__ (self):
        container  = CTK.Container()
        container += CTK.RawHTML(f"<h2>{_('Create a new configuration file:')}</h2>")

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

        conf_file = CTK.cfg.file
        notice = CTK.Notice ('warning')
        notice += CTK.RawHTML (f"<b>{_(WARNING_NOT_FOUND_1) % locals()}</b><br/>")
        notice += CTK.RawHTML (WARNING_NOT_FOUND_2)

        page = Page.Base(_('New Configuration File'), body_id='new-config', helps=HELPS)
        page += CTK.RawHTML(f"<h1>{_('Configuration File Not Found')}</h1>")
        page += notice
        page += CTK.Indenter (container)
        return page.Render()


CTK.publish (f'^{URL_BASE}', Render)
CTK.publish (f'^{URL_APPLY}', apply, method="POST")
