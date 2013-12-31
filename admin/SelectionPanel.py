# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
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
import string

HEADER = ['<script type="text/javascript" src="/CTK/js/jquery.cookie.js"></script>',
          '<script type="text/javascript" src="/static/js/SelectionPanel.js"></script>']

COOKIE_NAME_DEFAULT = "selection"

JS_INIT = """
  $('#%(id)s').SelectionPanel ('%(table_id)s', '%(content_id)s', '%(cookie)s', '%(cookie_domain)s', '%(web_empty)s');
"""

class SelectionPanel (CTK.Box):
    def __init__ (self, callback, content_id, web_url, web_empty, draggable=True, container=None, cookie_name=COOKIE_NAME_DEFAULT):
        CTK.Box.__init__ (self, {'class': 'selection-panel', 'style':'display:none;'})

        self.table       = CTK.SortableList (callback, container)
        self.content_id  = content_id
        self.web_url     = web_url
        self.web_empty   = web_empty
        self.draggable   = draggable
        self.cookie_name = cookie_name

        self += self.table

    def Add (self, id_content, url, content, draggable=True, extraClass=''):
        assert type(url) == str
        assert type(content) == list

        # Row ID
        row_id = ''.join([('_',x)[x in string.letters+string.digits] for x in url])

        # Row Content
        row_content = CTK.Box({'class': 'row_content ' + extraClass,
                               'pid':   id_content,
                               'url':   url})
        for w in content:
            row_content += w

        # Add to the table
        self.table += [None, row_content]
        self.table[-1].props['id'] = id_content
        self.table[-1][2].props['class'] = "nodrag nodrop"

        # Draggable
        if self.draggable and draggable:
            self.table[-1][1].props['class'] = 'dragHandle'
        else:
            self.table[-1][1].props['class'] = 'nodragHandle'
            self.table[-1].props['class'] = 'nodrag nodrop'

    def Render (self):
        render = CTK.Box.Render (self)

        props = {'id':            self.id,
                 'table_id':      self.table.id,
                 'content_id':    self.content_id,
                 'cookie':        self.cookie_name,
                 'cookie_domain': self.web_url,
                 'web_empty':     self.web_empty}

        render.js      += JS_INIT %(props)
        render.headers += HEADER

        return render
