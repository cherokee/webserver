# -*- coding: utf-8 -*-

# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2010-2014 Alvaro Lopez Ortega
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

import os
import sys
import types
import __builtin__

# Global
active_lang = ''

#
# Init
#
if not __builtin__.__dict__.has_key('_'):
    __builtin__.__dict__['_'] = lambda x: x

if not __builtin__.__dict__.has_key('N_'):
    __builtin__.__dict__['N_'] = lambda x: x

try:
    import gettext
except ImportError:
    pass


#
# Functions
#
def underscore_wrapper (x):
    # Feed unicode
    if type(x) != types.UnicodeType:
        x = unicode(x, 'UTF-8')

    # Translate using the original gettext function
    re = __builtin__.__dict__['_orig'] (x)

    # Transform the output to UTF-8
    if type(re) == types.UnicodeType:
        re = re.encode ('UTF-8')

    return re


def unicode_utf8_workaround():
    #  Wrap the _() function. Since Gettext internal catalog stores
    # Unicode values, we have to take care of using Unicode strings
    # rather than UTF-8, and to convert the translated strings back
    # to UTF-8 to be consumed by CTK. [workaround]
    #
    __builtin__.__dict__['_orig'] = __builtin__.__dict__['_']
    __builtin__.__dict__['_']     = underscore_wrapper


def install (*args, **kwargs):
    if not 'gettext' in sys.modules:
        return

    gettext.install (*args, **kwargs)

    # Initialize 'active_lang'
    for envar in ('LANGUAGE', 'LC_ALL', 'LC_MESSAGES', 'LANG'):
        val = os.environ.get(envar)
        if val:
            global active_lang
            active_lang = val
            break

    # Autoconversion unicode -> utf8 hack
    unicode_utf8_workaround()


def install_translation (propg, localedir, languages, *args, **kwargs):
    if not 'gettext' in sys.modules:
        return

    # Call gettext (returns a gettext.GNUTranslations obj)
    re = gettext.translation (propg, localedir, languages, *args, **kwargs)

    # Install the new gettext object
    re.install (unicode=True)

    # It worked, store the global
    if languages:
        global active_lang
        active_lang = languages[0]

    # Autoconversion unicode -> utf8 hack
    unicode_utf8_workaround()
