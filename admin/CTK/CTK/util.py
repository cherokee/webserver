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
import compiler

try:
    import json
except ImportError:
    import json_embedded as json

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


#
# Unicode, UTF-8
#
def to_utf8 (s, input_encoding='utf-8'):
    """Converts all the string entries of an structure to UTF-8. It
    supposes default system encoding to be UTF-8, so basically it
    converts Unicode to UTF-8 only"""

    if type(s) == types.StringType:
        if input_encoding == 'utf-8':
            return s
        return unicode (s, input_encoding).encode('utf-8')
    elif type(s) == types.UnicodeType:
        return s.encode('utf-8')
    elif type(s) == types.ListType:
        return [to_utf8(x) for x in s]
    elif type(s) == types.TupleType:
        return tuple([to_utf8(x) for x in s])
    elif type(s) == types.NoneType:
        return s
    elif type(s) == types.DictType:
        for k in s.keys():
            if type(k) in (types.StringType, types.UnicodeType):
                k = to_utf8(k)
            s[k] = to_utf8(s[k])
        return s

    return s


def to_unicode (s, input_encoding='utf-8'):
    """Converts all the string entries of an structure to Unicode. It
    supposes default system encoding to be UTF-8."""

    if type(s) == types.UnicodeType:
        return s
    elif type(s) == types.StringType:
        return unicode (s, input_encoding)
    elif type(s) == types.ListType:
        return [to_unicode(x) for x in s]
    elif type(s) == types.TupleType:
        return tuple([to_unicode(x) for x in s])
    elif type(s) == types.NoneType:
        return s
    elif type(s) == types.DictType:
        for k in s.keys():
            if type(k) in (types.StringType, types.UnicodeType):
                k = to_unicode(k)
            s[k] = to_unicode(s[k])
        return s

    return s


#
# Debug
#
def print_exception (output = sys.stderr):
    print >> output, traceback.format_exc()



#
# Safe data struct parsing
#
def data_eval (node_or_string):
    _safe_names = {'None': None, 'True': True, 'False': False}

    if isinstance(node_or_string, basestring):
        node_or_string = compiler.parse(node_or_string, mode='eval')

    if isinstance(node_or_string, compiler.ast.Expression):
        node_or_string = node_or_string.node

    def _convert(node):
        if isinstance(node, compiler.ast.Const) and isinstance(node.value, (basestring, int, float, long, complex)):
            return node.value
        elif isinstance(node, compiler.ast.Tuple):
            return tuple(map(_convert, node.nodes))
        elif isinstance(node, compiler.ast.List):
            return list(map(_convert, node.nodes))
        elif isinstance(node, compiler.ast.Dict):
            return dict((_convert(k), _convert(v)) for k, v in node.items)
        elif isinstance(node, compiler.ast.Name):
            if node.name in _safe_names:
                return _safe_names[node.name]
        elif isinstance(node, compiler.ast.UnarySub):
            return -_convert(node.expr)
        raise ValueError('malformed string')

    return _convert(node_or_string)

#
# Hash
#
try:
    # Python >= 2.5
    from hashlib import md5
except ImportError:
    # Python <= 2.4
    from md5 import md5
