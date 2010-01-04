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
from Container import Container
from HiddenField import HiddenField

class PropsTable (Table):
    def __init__ (self, **kwargs):
        Table.__init__ (self, **kwargs)
        self.current_row = 1

    def Add (self, title, widget, comment):
        widget_title   = RawHTML(title)
        widget_comment = RawHTML(comment)

        Table.__setitem__ (self, (self.current_row, 1),  [widget_title, widget])
        Table.__setitem__ (self, (self.current_row+1, 1), widget_comment)

        field = Table.__getitem__ (self, (self.current_row+1, 1))
        field['cellspan'] = "2"

        self.current_row += 2

class PropsTableAuto (PropsTable):
    def __init__ (self, url, **kwargs):
        PropsTable.__init__ (self, **kwargs)
        self._url      = url
        self.constants = {}

    def Add (self, title, widget, comment):
        submit = Submitter (self._url)

        if self.constants:
            box = Container()
            box += widget
            for key in self.constants:
                box += HiddenField ({'name': key, 'value': self.constants[key]})

            submit += box
        else:
            submit += widget

        return PropsTable.Add (self, title, submit, comment)

    def AddConstant (self, key, val):
        self.constants[key] = val
