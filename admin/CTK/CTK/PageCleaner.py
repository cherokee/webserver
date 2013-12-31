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

PAGE_CLEAN_DUP_BEGIN = "\n___MAY_BE_DUPPED_BEGIN___\n"
PAGE_CLEAN_DUP_END   = "\n___MAY_BE_DUPPED_END___\n"


def Uniq_Block (txt):
    ret  = PAGE_CLEAN_DUP_BEGIN
    ret += txt
    ret += PAGE_CLEAN_DUP_END
    return ret

def Postprocess (txt):
    return _remove_dupped_code (txt)



def _remove_dupped_code (txt):
    dups = {}

    while True:
        # Find begin and end
        n1 = txt.find(PAGE_CLEAN_DUP_BEGIN)
        if n1 == -1:
            return txt

        n2 = txt.find(PAGE_CLEAN_DUP_END)
        assert n2 != -1

        # Remove tags
        maybe_dupped = txt[n1+len(PAGE_CLEAN_DUP_BEGIN):n2]
        if maybe_dupped in dups:
            txt = txt[:n1] + txt[n2+len(PAGE_CLEAN_DUP_END):]
        else:
            txt = txt[:n1] + maybe_dupped + txt[n2+len(PAGE_CLEAN_DUP_END):]
            dups[maybe_dupped] = True
