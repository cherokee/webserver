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

import CTK

from consts import *
from ows_consts import *
from configured import *
from XMLServerDigest import XmlRpcServer


class Menu (CTK.Box):
    def __init__ (self, entries=[]):
        CTK.Box.__init__ (self, {'class': 'market-menu'})
        self.entries = entries[:]

    def __iadd__ (self, entry):
        self.entries.append(entry)
        return self

    def Render (self):
        # Build
        for entry in self.entries:
            if isinstance(entry, CTK.Widget):
                CTK.Box.__iadd__ (self, CTK.Box ({'class': 'market-menu-item'}, entry))
            else:
                CTK.Box.__iadd__ (self, CTK.Box ({'class': 'market-menu-item'}, CTK.RawHTML (entry)))

            # Separator
            if self.entries.index(entry) != len(self.entries)-1:
                CTK.Box.__iadd__ (self, CTK.Box ({'class': 'market-menu-sep'}, CTK.RawHTML (" › ")))

        # Render
        return CTK.Box.Render (self)
