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

from Widget import Widget, RenderResponse
from Container import Container


class TableField (Container):
    def __init__ (self, widget=None):
        Container.__init__ (self)
        self._props = {}
        self._tag   = 'td'

        if widget:
            Container.__add__ (self, widget)

    def Render (self):
        # Build tag props
        props = ''
        for p in self._props:
            value = self._props[p]
            if value:
                props += ' %s="%s"' %(p, value)
            else:
                props += ' %s' %(p)

        # Render content
        render = Container.Render(self)

        # Output
        html  = '<%s%s>' % (self._tag, props)
        html += render.html
        html += '</%s>' % (self._tag)

        render.html = html
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
            raise IndexError, "Column number out of bounds (get)"
        if row > self.last_row:
            raise IndexError, "Row number out of bounds (get)"

        return self.rows[row-1][col-1]

    def Render (self):
        props = self.kwargs.get('props')
        if props:
            render = RenderResponse('<table %s>' %(props))
        else:
            render = RenderResponse('<table>')

        for row in self.rows:
            render.html += '<tr>'
            for field in row:
                if field:
                    assert isinstance(field, TableField)
                    r = field.Render()
                    render += r
            render.html += '</tr>'

        render.html += '</table>'
        return render

    def set_header (self, row=True, column=False, num=1):
        if row:
            for field in self.rows[num-1]:
                field._tag = 'th'
            return

        if column:
            for row in self.rows:
                row[num-1]._tag = 'th'
            return


class TableFixed (Table):
    def __init__ (self, rows, cols):
        Table.__init__ (self)
        self._fixed_rows = rows
        self._fixed_cols = cols

    def __setitem__ (self, pos, item):
        row,col = pos

        if row > self._fixed_rows:
            raise IndexError, "Row number out of bounds"

        if col > self._fixed_cols:
            raise IndexError, "Column number out of bounds"

        return Table.__setitem__ (self, pos, item)
