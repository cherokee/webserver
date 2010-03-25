# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2009 Alvaro Lopez Ortega
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

from cgi import parse_qs
from urllib import unquote


class Post:
    def __init__ (self, raw=''):
        self._vars = {}

        tmp = parse_qs (raw, keep_blank_values=1)
        for key in tmp:
            self._vars[key] = []
            for n in range(len(tmp[key])):
                value = tmp[key][n]
                self._vars[key] += [unquote (value)]

    def _smart_chooser (self, key):
        if not key in self._vars:
            return None

        vals = filter(lambda x: len(x)>0, self._vars[key])
        if not len(vals) > 0:
            return self._vars[key][0]

        return vals[0]

    def get_val (self, key, not_found=None):
        tmp = self._smart_chooser(key)
        if not tmp:
            return not_found
        return tmp

    def get_all (self, key, not_found=[]):
        if not key in self._vars:
            return not_found[:]

        return filter(lambda x: len(x)>0, self._vars[key])

    def pop (self, key, not_found=None):
        val = self._smart_chooser(key)
        if val == None:
            return not_found
        if key in self._vars:
            del(self[key])
        return val

    def keys (self):
        return self._vars.keys()

    # Relay on the internal array methods
    #
    def __getitem__ (self, key):
        return self._vars[key]

    def __setitem__ (self, key, val):
        self._vars[key] = val

    def __delitem__ (self, key):
        del (self._vars[key])

    def __len__ (self):
        return len(self._vars)

    def __iter__ (self):
        return iter(self._vars)

    def __str__ (self):
        return str(self._vars)
