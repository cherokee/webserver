# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2011 Alvaro Lopez Ortega
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
import time

file = None
buf  = ''

def reset():
    global file, buf
    file = None
    buf  = ''

def set_file (filepath):
    global buf
    global file

    assert type(filepath) == str

    file = open (filepath, 'w+')
    os.chmod (filepath, 0600)

    file.write (buf)
    buf = ''

def log (txt):
    global buf
    global file

    now = time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime())

    entry = '[%s] %s' %(now, txt)
    if not entry.endswith('\n'):
        entry += '\n'

    if not file:
        buf += entry
    else:
        file.write (entry)
        file.flush()

def get_full_log():
    txt = ''
    if file:
        file.seek(0)
        txt += file.read()

    txt += buf
    return txt

