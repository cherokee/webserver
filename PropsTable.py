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

from Table import Table
from RawHTML import RawHTML
from Submitter import Submitter

class PropsTable (Table):
    def __init__ (self, rows, **kwargs):
        Table.__init__ (self, 2, rows, **kwargs)
        self.current_row = 1

    def __setitem__ (self, title, widget):
        title_widget = RawHTML(title)

        Table.__setitem__ (self, (self.current_row, 1), title_widget)
        Table.__setitem__ (self, (self.current_row, 2), widget)

        self.current_row += 1

class PropsTableAuto (PropsTable):
    def __init__ (self, rows, url, **kwargs):
        PropsTable.__init__ (self, rows, **kwargs)
        self._url = url

    def __setitem__ (self, title, widget):
        submit = Submitter (self._url)
        submit += widget

        return PropsTable.__setitem__ (self, title, submit)
