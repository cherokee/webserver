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

widget_uniq_id = 1;


class RenderResponse:
    def __init__ (self, html='', js='', headers=[]):
        self.html    = html
        self.js      = js
        self.headers = headers

    def __add__ (self, other):
        i = RenderResponse()

        i.html    = self.html    + other.html
        i.js      = self.js      + other.js
        i.headers = self.headers + other.headers

        return i


class Widget:
    def __init__ (self):
        global widget_uniq_id;

        widget_uniq_id += 1;
        self.uniq_id = widget_uniq_id;

    def Render (self):
        raise NotImplementedError, "Pure Virtual Method"
