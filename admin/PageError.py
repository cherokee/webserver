# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
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

from CTK.i18n import _, N_
import os
import ast
import CTK

from urllib.parse import quote
from configured import *

ERROR_LAUNCH_URL_ADMIN = N_('The server suggests to check <a href="%s">this page</a>. Most probably the problem can be solved in there.')

class PageErrorLaunch (CTK.Page):
    def __init__ (self, error_str, **kwargs):
        srcdir = os.path.dirname (os.path.realpath (__file__))
        theme_file = os.path.join (srcdir, 'exception.html')

        # Set up the template
        template = CTK.Template (filename = theme_file)
        template['body_props'] = ' id="body-error"'

        # Parent's constructor
        CTK.Page.__init__ (self, template, **kwargs)

        # Error parse
        self._error: dict[str, str] | None = None
        self._error_raw = error_str

        for line in error_str.split("\n"):
            if not line:
                continue

            if line.startswith("{'type': "):
                parsed = ast.literal_eval(line)
                if isinstance(parsed, dict):
                    self._error = {str(key): str(value)
                                   for key, value in parsed.items()}
                    break

        # Build the page content
        template['title'] = _('Server Launch Error')

        if self._error:
            self._build_error_py()
        else:
            self._build_error_raw()

    def _build_error_py (self):
        error = self._error
        if error is None:
            return
        self += CTK.RawHTML (f"<h1>{error['title']}</h1>")
        self += CTK.Box ({'class': 'description'}, CTK.RawHTML(error.get('description','')))

        admin_url = error.get('admin_url')
        if admin_url:
            self += CTK.Box ({'class': 'admin-url'}, CTK.RawHTML(_(ERROR_LAUNCH_URL_ADMIN)%(admin_url)))

        debug = error.get('debug')
        if debug:
            self += CTK.Box ({'class': 'debug'}, CTK.RawHTML(debug))

        backtrace = error.get('backtrace')
        if backtrace:
            self += CTK.Box ({'class': 'backtrace'}, CTK.RawHTML(backtrace))

        self += CTK.Box ({'class': 'time'}, CTK.RawHTML(error.get('time','')))

    def _build_error_raw (self):
        self += CTK.RawHTML (f"<h1>{_('Server Launch Error')}</h1>")
        self += CTK.Box ({'class': 'description'}, CTK.RawHTML(_('Something unexpected just happened.')))
        self += CTK.Box ({'class': 'backtrace'},   CTK.RawHTML(self._error_raw))



NOT_WRITABLE_F_TITLE = N_('The configuration file <code>%s</code> cannot be modified.')
NOT_WRITABLE_F_1     = N_('You have to change its permissions in order to allow cherokee-admin to work with it. You can try to fix it by changing the file permissions:')
NOT_WRITABLE_F_2     = N_('or by launching cherokee-admin as root.')

NOT_WRITABLE_D_TITLE = N_('The specified configuration file <code>%s</code> is a directory.')
NOT_WRITABLE_D_1     = N_('You must change the name in order to allow cherokee-admin to work with it. You can try to fix it by renaming the directory so the file can be created: ')
NOT_WRITABLE_D_2     = N_('or by launching cherokee-admin specifying a different configuration file.')

class ConfigNotWritable (CTK.Page):
    def __init__ (self, file, **kwargs):
        srcdir = os.path.dirname (os.path.realpath (__file__))
        theme_file = os.path.join (srcdir, 'exception.html')

        # Set up the template
        template = CTK.Template (filename = theme_file)
        template['body_props'] = ' id="body-error"'
        template['title']      = _('Configuration file is not writable')

        # Parent's constructor
        CTK.Page.__init__ (self, template, **kwargs)

        # Body
        if os.path.isfile (file):
            self += CTK.RawHTML (f"<h1>{_('Configuration file is not writable')}</h1>")
            self += CTK.RawHTML (f'<p><strong>{NOT_WRITABLE_F_TITLE % file}</strong></p>')
            self += CTK.RawHTML (f'<p>{NOT_WRITABLE_F_1}</p>')
            self += CTK.RawHTML (f'<div class="shell">chmod u+w {file}</div>')
            self += CTK.RawHTML (f'<p>{NOT_WRITABLE_F_2}</p>')
        else:
            self += CTK.RawHTML (f"<h1>{_('Incorrect configuration file specified')}</h1>")
            self += CTK.RawHTML (f'<p><strong>{NOT_WRITABLE_D_TITLE % file}</strong></p>')
            self += CTK.RawHTML (f'<p>{NOT_WRITABLE_D_1}</p>')
            self += CTK.RawHTML (f'<div class="shell">mv {file} {file}.dir</div>')
            self += CTK.RawHTML (f'<p>{NOT_WRITABLE_D_2}</p>')


def NotWritable (file):
    return ConfigNotWritable (file).Render()



RESOURCE_MISSING_TITLE = N_('Could not find a Cherokee resource: <code>%s</code>')
RESOURCE_MISSING       = N_('You may need to reinstall the Web Server in order to sort out this issue.')

class ResourceMissingError (CTK.Page):
    def __init__ (self, path, **kwargs):
        srcdir = os.path.dirname (os.path.realpath (__file__))
        theme_file = os.path.join (srcdir, 'exception.html')

        # Set up the template
        template = CTK.Template (filename = theme_file)
        template['body_props'] = ' id="body-error"'
        template['title']      = _('Cherokee resource is missing')

        # Parent's constructor
        CTK.Page.__init__ (self, template, **kwargs)

        # Body
        self += CTK.RawHTML (f"<h1>{template['title']}</h1>")
        self += CTK.RawHTML (f'<p><strong>{_(RESOURCE_MISSING_TITLE) % path}</strong>')
        self += CTK.RawHTML (f'<p>{RESOURCE_MISSING}</p>')

def ResourceMissing (path):
    return ResourceMissingError(path).Render()



ANCIENT_CONFIG_TITLE = N_('Cherokee-admin has detected a very old configuration file.')
ANCIENT_CONFIG       = N_('Most probably cherokee-admin is trying to read an old configuration file. Please, remove it so cherokee-admin can create a new one with the right format.')

class AncientConfigError (CTK.Page):
    def __init__ (self, path, **kwargs):
        srcdir = os.path.dirname (os.path.realpath (__file__))
        theme_file = os.path.join (srcdir, 'exception.html')

        # Set up the template
        template = CTK.Template (filename = theme_file)
        template['body_props'] = ' id="body-error"'
        template['title']      = _('Detected an Ancient Configuration File')

        # Parent's constructor
        CTK.Page.__init__ (self, template, **kwargs)

        # Body
        self += CTK.RawHTML (f"<h1>{_('Ancient Configuration File')}</h1>")
        self += CTK.RawHTML (f'<p><strong>{_(ANCIENT_CONFIG_TITLE)}</strong>')
        self += CTK.RawHTML (f'<p>{ANCIENT_CONFIG}</p>')
        self += CTK.RawHTML (f"<p><pre>rm -f '{path}'</pre></p>")

def AncientConfig (file):
    return AncientConfigError(file).Render()
