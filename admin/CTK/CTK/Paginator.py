# -*- coding: utf-8 -*-
#
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

import re
import string

from Box import Box
from Widget import Widget, RenderResponse
from RawHTML import RawHTML
from Refreshable import Refreshable
from Container import Container
from Link import Link
from Server import request

FOOTER_OPTIONS   = 3
SHOW_FOOTER_1PAG = False

paginator_last_refresh = {}

class Paginator_Footer (Box):
    def __init__ (self, results_num, page_num, items_per_page, total_pages, refreshable):
        Box.__init__ (self, {'class': 'paginator-footer'})

        # Get the base URL
        url = refreshable.url
        while url[-1] in string.digits+'/':
            url = url[:-1]

        # Reckon the range
        extra = 0
        if page_num + FOOTER_OPTIONS > total_pages:
            extra += abs (total_pages - (page_num + FOOTER_OPTIONS))
        if page_num - FOOTER_OPTIONS < 0:
            extra += abs (page_num - (FOOTER_OPTIONS + 1))

        chunk_raw = range(page_num - (FOOTER_OPTIONS + extra), page_num + FOOTER_OPTIONS + extra + 1)
        chunk     = filter (lambda x: x >= 0 and x < total_pages, chunk_raw)

        # Render it
        if page_num != 0:
            url = '%s/%d' %(refreshable.url, page_num-1)
            link = Link (None, RawHTML (_("Previous")), {'class': 'paginator-footer-prev'})
            link.bind ('click', refreshable.JS_to_refresh(url=url))
            self += link

        indexes = Container()
        for p in chunk:
            if p == page_num:
                indexes += RawHTML ("%d"%(p+1))
            else:
                url = '%s/%d' %(refreshable.url, p)
                link = Link (None, RawHTML ("%d"%(p+1)), {'class': 'paginator-footer-page'})
                link.bind ('click', refreshable.JS_to_refresh(url=url))
                indexes += link

            if chunk.index(p) < len(chunk)-1:
                indexes += RawHTML (", ")

        self += indexes

        if page_num < total_pages-1:
            url = '%s/%d' %(refreshable.url, page_num+1)
            link = Link (None, RawHTML (_("Next")), {'class': 'paginator-footer-next'})
            link.bind ('click', refreshable.JS_to_refresh(url=url))
            self += link


class Paginator_Refresh (Widget):
    def __init__ (self, page_num, items_per_page, refreshable):
        Widget.__init__ (self)
        self.items            = refreshable.items
        self.refreshable      = refreshable
        self.items_per_page   = items_per_page
        self.show_footer_1pag = SHOW_FOOTER_1PAG

        global paginator_last_refresh

        tmp = re.findall ('^%s/(\d+)$'%(refreshable.url), request.url)
        if tmp:
            self.page_num = int(tmp[0])
            paginator_last_refresh [refreshable.id] = self.page_num
        elif paginator_last_refresh.has_key (refreshable.id):
            self.page_num = paginator_last_refresh [refreshable.id]
        else:
            self.page_num = page_num

    def Render (self):
        render = Widget.Render (self)

        # Total pages
        tmp = len(self.items)/float(self.items_per_page)
        if int(tmp) < tmp:
            total_pags = int(tmp) + 1
        else:
            total_pags = int(tmp) or 1


        if total_pags > 1 or self.show_footer_1pag:
            title = Box ({'class': 'paginator-counter'}, RawHTML ("Page %d of %d" %(self.page_num+1, total_pags)))
            render += title.Render()

        # Content
        range_start = self.items_per_page * self.page_num
        range_end   = self.items_per_page * (self.page_num + 1)

        for item in self.items[range_start:range_end]:
            render += item.Render()

        # Add footer
        if total_pags > 1 or self.show_footer_1pag:
            footer = Paginator_Footer (len(self.items), self.page_num, self.items_per_page, total_pags, self.refreshable)
            render += footer.Render()

        return render


class Paginator (Refreshable):
    """
    Widget that displays its contents in a paginated manner if
    neccesary. If more than one page is generated, a navigation footer
    will be generated.

    Arguments:

        name: a name to append to 'refreshable-' to generate the
            automatically assigned id for the container. Remember that
            DOM elements must have unique identifiers.

        page_num: sub-page to open as default.

        items_per_page: number of contained CTK widgets to show at
            once.

    Examples:
        pags = CTK.Paginator('paginated-container', items_per_page=2)
        for x in range(5):
            pags += CTK.RawHTML('<p>%s.</p>' %(x))
    """
    def __init__ (self, name, page_num=0, items_per_page=20):
        Refreshable.__init__ (self, {'id': 'refreshable-%s'%(name)})

        self.items          = []
        self.page_num       = page_num
        self.items_per_page = items_per_page

        self.register (lambda: Paginator_Refresh (page_num, items_per_page, self).Render())

    def __iadd__ (self, widget):
        assert isinstance(widget, Widget)
        self.items.append (widget)
        return self
