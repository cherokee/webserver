# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2009-2010 Alvaro Lopez Ortega
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

import re

try:
    import json
except ImportError:
    import json_embedded as json

#
# Strings
#
def formater (string, props):
    """ This function does a regular substitution 'str%(dict)' with a
    little difference. It takes care of the escaped percentage chars,
    so strings can be replaced an arbitrary number of times."""

    s2 = ''
    n  = 0
    while n < len(string):
        if n<len(string)-1 and string[n] == string[n+1] == '%':
            s2 += '%%%%'
            n  += 2
        else:
            s2 += string[n]
            n  += 1

    return s2 %(props)

#
# HTML Tag properties
#
def props_to_str (props):
    assert type(props) == dict

    tmp = []
    for p in props:
        val = props[p]
        if val:
            tmp.append ('%s="%s"'%(p, val))
        else:
            tmp.append (p)

    return ' '.join(tmp)

#
# Copying and Cloning
#
def find_copy_name (orig, names):
    # Clean up name
    cropped = re.search (r' Copy( \d+|)$', orig) != None
    if cropped:
        orig = orig[:orig.rindex(' Copy')]

    # Find higher copy
    similar = filter (lambda x: x.startswith(orig), names)
    if '%s Copy'%(orig) in similar:
        higher = 1
    else:
        higher = 0

    for tmp in [re.findall(r' Copy (\d)+', x) for x in similar]:
        if not tmp: continue
        higher = max (int(tmp[0]), higher)

    # Copy X
    if higher == 0:
        return '%s Copy' %(orig)

    return '%s Copy %d' %(orig, higher+1)

#
# JSon
#
def json_dump (obj):
    # Python 2.6, and json_embeded
    if hasattr (json, 'dumps'):
        return json.dumps (obj)

    # Python 2.5
    return json.write(obj)
