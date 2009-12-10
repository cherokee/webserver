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

from Widget import Widget
from Container import Container

class TableField (Container):
    def __init__ (self, widget=None):
        Container.__init__ (self)
        self._props = {}
        self._tag   = 'td'

        if widget:
            self += widget

    def Render (self):
        render = '<%s' % (self._tag)
        for prop in self._props:
            value = self._props[prop]
            if value:
                render += ' %s="%s"' %(prop, value)
            else:
                render += ' %s' %(prop)

        container = Container.Render(self)
        render += '>%s</%s>' %(container, self._tag)
        return render

    def __setitem__ (self, prop, value):
        assert type(prop)  == str
        assert type(value) == str

        self._props[prop] = value


class Table (Widget):
    def __init__ (self, **kwargs):
        Widget.__init__ (self)
        self.kwargs   = kwargs
        self.last_col = 0
        self.last_row = 0
        self.rows     = []

    def __add_row (self):
        self.rows.append ([None] * self.last_col)
        self.last_row += 1

    def __add_col (self):
        for row in self.rows:
            row.append (None)
        self.last_col += 1
    
    def __grow_if_needed (self, row, col):
        if col < 1:
            raise IndexError, "Column number out of bounds"
        if row < 1:
            raise IndexError, "Row number out of bounds"

        while row > self.last_row:
            self.__add_row()

        while col > self.last_col:
            self.__add_col()

    def __setitem_single (self, pos, widget):
        assert isinstance(widget, Widget)
        assert type(pos) == tuple

        # Check table bounds
        row,col = pos
        self.__grow_if_needed (row, col)

        # The field object
        field = TableField (widget)

        # Set the element
        table_row = self.rows[row-1]
        table_row[col-1] = field

    def __setitem_list (self, pos, wid_list):
        row,col = pos
        for widget in wid_list:
            self.__setitem_single ((row,col), widget)
            col += 1

    def __setitem__ (self, pos, item):
        if type(item) == list:
            return self.__setitem_list (pos, item)
        else:
            return self.__setitem_single (pos, item)

    def __getitem__ (self, pos):
        row,col = pos

        if col > self.last_col:
            raise IndexError, "Column number out of bounds"
        if row > self.last_row:
            raise IndexError, "Row number out of bounds"

        return self.rows[row-1][col-1]
            
    def Render (self):
        props = self.kwargs.get('props')
        if props:
            render = '<table %s>' %(props)
        else:
            render = '<table>'

        for row in self.rows:
            render += '<tr>'
            for field in row:
                if field:
                    assert isinstance(field, TableField)
                    render += field.Render()
            render += '</tr>'

        render += '</table>'
        return render
