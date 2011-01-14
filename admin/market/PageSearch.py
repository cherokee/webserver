# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2011 Alvaro Lopez Ortega
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
import CTK

from consts import *
from ows_consts import *
from configured import *

from Util import *
from Menu import Menu
from XMLServerDigest import XmlRpcServer


def Search_Apply():
    string = CTK.post.get_val('search')
    if not string:
        return CTK.cfg_reply_ajax_ok()
    return {'ret':'ok', 'redirect': '%s/%s' %(URL_SEARCH, string)}


class Search_Widget (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'market-search-box'})
        submit = CTK.Submitter (URL_SEARCH_APPLY)
        submit += CTK.TextField ({'name': 'search', 'optional_string': _('Search'), 'optional': True, 'class': 'filter'})
        self += submit

class Search:
    def format_func (self, info):
        pags = CTK.Paginator('search-results', items_per_page=10)

        # Title
        cont = CTK.Container()
        cont += CTK.RawHTML ("<h2>%d Result%s for: %s</h2>" %(len(info), ('','s')[len(info)>1], self.text_search))
        cont += pags

        # List of apps
        for app in info:
            pags += RenderApp (app)

        return cont.Render().toStr()

    def __call__ (self):
        page = Page_Market(_('Search Result'))

        mainbox = CTK.Box ({'class': 'market-main-area'})
        mainbox += CTK.RawHTML("<h1>%s</h1>" %(_('Market')))


        # Parse the URL
        tmp = re.findall ('^%s/(.+)$' %(URL_SEARCH), CTK.request.url)
        if not tmp:
            page += CTK.RawHTML ('<h2>%s</h2>' %(_("Empty Search")))
            return page.Render()

        self.text_search = tmp[0]

        # Menu
        menu = Menu([CTK.Link(URL_MAIN, CTK.RawHTML (_('Market Home')))])
        menu += "%s %s"%(_("Search"), self.text_search)
        page.mainarea += menu

        # Perform the search
        page.mainarea += CTK.XMLRPCProxy (
            name = 'cherokee-market-search',
            xmlrpc_func = lambda: XmlRpcServer (OWS_APPS).lookup_application (CTK.util.to_unicode (self.text_search), OWS_Login.login_session),
            format_func = self.format_func,
            debug = OWS_DEBUG)

        return page.Render()


CTK.publish ('^%s$'%(URL_SEARCH_APPLY), Search_Apply, method="POST")
CTK.publish ('^%s' %(URL_SEARCH),       Search)
