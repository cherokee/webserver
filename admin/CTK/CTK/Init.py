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

import sys
import builtins
import importlib

#
# Built-in functions
#

def builtin_any (iterable):
    for element in iterable:
        if element:
            return True
    return False

def builtin_all (iterable):
    for element in iterable:
        if not element:
            return False
    return True


def Init():
    # Deals with UTF-8
    if sys.getdefaultencoding() != 'utf-8':
        importlib.reload (sys)
        sys.setdefaultencoding('utf-8')

    # Add a few missing functions
    if 'any' not in builtins.__dict__:
        builtins.__dict__['any'] = builtin_any

    if 'all' not in builtins.__dict__:
        builtins.__dict__['all'] = builtin_all
