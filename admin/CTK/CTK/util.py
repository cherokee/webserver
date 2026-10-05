# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2009-2014 Alvaro Lopez Ortega
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
import sys
import types
import traceback
import ast


def cmp(left, right):
    return (left > right) - (left < right)

try:
    import json
except ImportError:
    from . import json_embedded as json

#
# Strings
#
def formatter (string, props):
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
            tmp.append (f'{p}="{val}"')
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
    similar = [x for x in names if x.startswith(orig)]
    if f'{orig} Copy' in similar:
        higher = 1
    else:
        higher = 0

    for tmp in [re.findall(r' Copy (\d)+', x) for x in similar]:
        if not tmp: continue
        higher = max (int(tmp[0]), higher)

    # Copy X
    if higher == 0:
        return f'{orig} Copy'

    return f'{orig} Copy {int(higher + 1)}'

#
# JSon
#
def json_dump (obj):
    # Legacy JSON embedding compatibility.
    if hasattr (json, 'dumps'):
        return json.dumps (obj)

    # Legacy fallback retained for old configuration data.
    return json.write(obj)


#
# Unicode, UTF-8
#
def to_utf8 (s, input_encoding='utf-8'):
    """Converts all the string entries of an structure to UTF-8. It
    supposes default system encoding to be UTF-8, so basically it
    converts Unicode to UTF-8 only"""

    if type(s) == bytes:
        return s.decode(input_encoding)
    elif type(s) == str:
        return s
    elif type(s) == list:
        return [to_utf8(x) for x in s]
    elif type(s) == tuple:
        return tuple([to_utf8(x) for x in s])
    elif type(s) == type(None):
        return s
    elif type(s) == dict:
        for k in list(s.keys()):
            if type(k) in (bytes, str):
                k = to_utf8(k)
            s[k] = to_utf8(s[k])
        return s

    return s


def to_unicode (s, input_encoding='utf-8'):
    """Converts all the string entries of an structure to Unicode. It
    supposes default system encoding to be UTF-8."""

    if type(s) == str:
        return s
    elif type(s) == bytes:
        return str (s, input_encoding)
    elif type(s) == list:
        return [to_unicode(x) for x in s]
    elif type(s) == tuple:
        return tuple([to_unicode(x) for x in s])
    elif type(s) == type(None):
        return s
    elif type(s) == dict:
        for k in list(s.keys()):
            if type(k) in (bytes, str):
                k = to_unicode(k)
            s[k] = to_unicode(s[k])
        return s

    return s


#
# Debug
#
def print_exception (output = sys.stderr):
    print(traceback.format_exc(), file=output)



#
# Safe data struct parsing
#
def data_eval (node_or_string):
    _safe_names = {'None': None, 'True': True, 'False': False}

    if isinstance(node_or_string, str):
        node_or_string = ast.parse(node_or_string, mode='eval').body

    def _convert(node):
        if isinstance(node, ast.Constant) and isinstance(node.value, (str, int, float, complex)):
            return node.value
        elif isinstance(node, ast.Tuple):
            return tuple(map(_convert, node.elts))
        elif isinstance(node, ast.List):
            return list(map(_convert, node.elts))
        elif isinstance(node, ast.Dict):
            return dict((_convert(k), _convert(v)) for k, v in zip(node.keys, node.values))
        elif isinstance(node, ast.Name) and node.id in _safe_names:
            return _safe_names[node.id]
        elif isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
            return -_convert(node.operand)
        raise ValueError('malformed string')

    return _convert(node_or_string)

#
# Hash
#
from hashlib import md5
