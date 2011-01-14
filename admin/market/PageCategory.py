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
import Page
import OWS_Login

from consts import *
from ows_consts import *
from configured import *

from Util import *
from Menu import Menu
from XMLServerDigest import XmlRpcServer


class Categories_Widget (CTK.Box):
    cached = None

    def format_func (self, info):
        # Format the list
        cat_list = CTK.List()
        for cat in info:
            link = CTK.Link ('%s/%s/%s' %(URL_CATEGORY, cat['category_id'], cat['category_name']), CTK.RawHTML(cat['category_name']))
            cat_list += link

        # Save a copy
        render_str = cat_list.Render().toStr()
        if len(info) > 2:
            Categories_Widget.cached = render_str
        return render_str

    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'market-categories-list'})
        self += CTK.RawHTML ('<h3>%s</h3>' %(_('Categories')))

        if self.cached:
            self += CTK.RawHTML (self.cached)
        else:
            self += CTK.XMLRPCProxy (name = 'cherokee-market-categories',
                                     xmlrpc_func = lambda: XmlRpcServer(OWS_APPS).list_categories(),
                                     format_func = self.format_func,
                                     debug = OWS_DEBUG)


class Category:
    def format_func (self, info):
        pags = CTK.Paginator('category-results', items_per_page=10)
        for app in info:
            pags += RenderApp (app)

        return pags.Render().toStr()

    def __call__ (self):
        page = Page_Market(_('Category'))

        # Parse URL
        tmp = re.findall ('^%s/(\d+)/?(.*)$' %(URL_CATEGORY), CTK.request.url)
        if not tmp:
            page += CTK.RawHTML ('<h2>%s</h2>' %(_("Empty Category")))
            return page.Render()

        self.categoty_num  = tmp[0][0]
        self.category_name = tmp[0][1]

        # Menu
        menu = Menu([CTK.Link(URL_MAIN, CTK.RawHTML (_('Market Home')))])
        menu += "%s: %s" %(_('Category'), self.category_name)
        page.mainarea += menu

        # Perform the search
        page.mainarea += CTK.XMLRPCProxy (name = 'cherokee-market-category',
                                 xmlrpc_func = lambda: XmlRpcServer(OWS_APPS).list_apps_in_category (self.categoty_num, OWS_Login.login_session),
                                 format_func = self.format_func,
                                 debug = OWS_DEBUG)
        return page.Render()


CTK.publish ('^%s' %(URL_CATEGORY),     Category)
