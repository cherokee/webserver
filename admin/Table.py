SUBMIT_GENERIC = '<input type="submit" value="%s" />'
SUBMIT_ADD   = SUBMIT_GENERIC % ('Add')
SUBMIT_DEL   = SUBMIT_GENERIC % ('Del')
SUBMIT_CLONE = SUBMIT_GENERIC % ('Clone')

class Table:
    def __init__ (self, cols, title_top=0, title_left=0, style='', header_style=''):
        self._cols         = cols
        self._title_top    = title_top
        self._title_left   = title_left
        self._style        = style
        self._header_style = header_style
        self._content      = []

    def __add__ (self, entry):
        assert (type(entry) == tuple)

        new_row = [''] * self._cols
        for i in range(len(entry)):
            new_row[i] = entry[i]

        self._content.append (new_row)
        return self

    def __str__ (self):
        if self._style:
            txt = '<table %s >\n' % (self._style)
        else:
            txt = '<table>\n'

        for nrow in range(len(self._content)):
            line = ''
            row = self._content[nrow]
            for nentry in range(len(row)):
                entry = row[nentry]
                if nrow < self._title_top or \
                   nentry < self._title_left:
                    line += '<th %s >%s</th>' % (self._header_style, str(entry))
                else:
                    line += '<td>%s</td>' % (str(entry))
            txt += '\t<tr>%s</tr>\n' % (line)
        txt += '</table>\n'
        return txt

    def Set (self, x, y, txt):
        self._content[y][x] = txt


