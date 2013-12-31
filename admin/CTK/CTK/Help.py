# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2010-2014 Alvaro Lopez Ortega
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


class HelpEntry (Widget):
    def __init__ (self, title, ref):
        Widget.__init__ (self)
        self.title = title
        self.ref   = ref

    def Render (self):
        if '://' in self.ref:
            url = self.ref
        else:
            url = "/help/%s.html" %(self.ref)

        render = Widget.Render(self)
        render.html = '<div class="help_entry"><a href="%s" target="cherokee_help">%s</a></div>' %(url, self.title)
        return render

    def __repr__ (self):
         return "<CTK.Help.HelpEntry: '%s', '%s', id=%d>"%(self.title, self.ref, id(self))


class HelpGroup (Widget):
    def __init__ (self, name, group=[]):
        Widget.__init__ (self)
        self.name    = name
        self.entries = []

        for entry in group:
            self += entry

    def __add__ (self, entry):
        assert (isinstance(entry, HelpEntry) or
                isinstance(entry, HelpGroup))

        # Add it
        self.entries.append (entry)
        return self

    def Render (self):
        render = Widget.Render(self)
        for entry in self.entries:
            render += entry.Render()

        render.html = '<div class="help_group" id="help_group_%s">%s</div>' %(self.name, render.html)
        return render

    def __repr__ (self):
        txt = ', '.join([e.__repr__() for e in self.entries])
        return "<CTK.Help.HelpGroup: id=%d, %s>"%(id(self), txt)

    def toJSON (self):
        all = []
        for entry in self.entries:
            if isinstance(entry, HelpEntry):
                all.append ((entry.title, entry.ref))
            else:
                all += entry.toJSON()
        return all


class HelpMenu (Widget):
    def __init__ (self, helps=None):
        Widget.__init__ (self)

        if not helps:
            self.helps = []
        else:
            self.helps = helps[:]

    def __add__ (self, helps):
        if type(helps) == list:
            for entry in helps:
                self._add_single (entry)
        else:
            self._add_single (entry)
        return self

    def _add_single (self, entry):
        assert (isinstance (entry, HelpEntry) or
                isinstance (entry, HelpGroup))
        self.helps.append (entry)

    def Render (self):
        # Empty response
        render = Widget.Render(self)

        # Render the help entries
        for entry in self.helps:
            render.html += entry.Render().html

        # Wrap the list of entries
        render.html = '<div class="help">%s</div>' %(render.html)
        return render

