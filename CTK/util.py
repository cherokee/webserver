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
