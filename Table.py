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
        self._props = None
        self._tag   = 'td'

        if widget:
            self += widget

    def Render (self):
        render = '<%s' % (self._tag)
        if self._props:
            render += ' %s' %(self._props)

        container = Container.Render(self)
        render += '>%s</%s>' %(container, self._tag)
        return render

    def SetProperties (self, props):
        assert type(props) == str
        self._props = props


class Table (Widget):
    def __init__ (self, cols, rows, **kwargs):
        Widget.__init__ (self)
        self.col_n  = cols
        self.row_n  = rows
        self.kwargs = kwargs

        # Build empty array
        self.rows = []
        for y_i in range(rows):
            self.rows.append ([None] * cols)

    def __setitem_single (self, pos, widget):
        assert isinstance(widget, Widget)
        assert type(pos) == tuple

        # Check table bounds
        row,col = pos
        if col-1 > self.col_n:
            raise IndexError, "Column number out of bounds"
        if row-1 > self.row_n:
            raise IndexError, "Row number out of bounds"
        
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
            
    def Render (self):
        props = self.kwargs.get('props')
        if props:
            render = '<table %s>' %(props)
        else:
            render = '<table>'

        for row in self.rows:
            render += '<tr>'
            for field in row:
                assert isinstance(field, TableField)
                render += field.Render()
            render += '</tr>'

        render += '</table>'
        return render
        
