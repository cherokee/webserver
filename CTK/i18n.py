# -*- coding: utf-8 -*-

# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
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

import sys
import __builtin__

# Global
active_lang = ''

#
# Init
#
try:
    import gettext
except ImportError:
    __builtin__.__dict__['_'] = lambda x: x

if not __builtin__.__dict__.has_key('N_'):
    __builtin__.__dict__['N_'] = lambda x: x


#
# Functions
#
def install (*args, **kwargs):
    if not 'gettext' in sys.modules:
        return
    return gettext.install (*args, **kwargs)

def translation (propg, localedir, languages, *args, **kwargs):
    if not 'gettext' in sys.modules:
        return

    # Call gettext
    re = gettext.translation (propg, localedir, languages, *args, **kwargs)

    # It worked, store the global
    if languages:
        global active_lang
        active_lang = languages[0]

    return re
