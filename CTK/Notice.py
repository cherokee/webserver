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

NOTICE_TYPES = ['information', 'important-information',
                'warning', 'error', 'offline', 'online']

HTML = """
<div class="dialog-%(klass)s" id="%(id)s">
%(text)s
</div>
"""

class Notice (Widget):
    def __init__ (self, text, klass='information'):
        assert klass in NOTICE_TYPES

        Widget.__init__ (self)
        self.text  = text
        self.klass = klass

    def Render (self):
        render = Widget.Render (self)
        render.html += HTML %({'text': self.text, 'id': self.id, 'klass': self.klass})
        return render
