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

from Container import Container

NOTICE_TYPES = ['information', 'important-information',
                'warning', 'error', 'offline', 'online']

HTML = """
<div class="dialog-%(klass)s" id="%(id)s">
%(content)s
</div>
"""

class Notice (Container):
    def __init__ (self, klass='information'):
        Container.__init__ (self)

        assert klass in NOTICE_TYPES
        self.klass = klass

    def Render (self):
        render = Container.Render (self)

        render.html = HTML %({'klass':   self.klass,
                              'id':      self.id,
                              'content': render.html})
        return render
