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
import Distro
import OWS_Login

from consts import *
from ows_consts import *
from configured import *

from Util import *
from Menu import Menu

class Categories_Widget (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'cherokee-market-categories'})
        index = Distro.Index()

        # Build category list
        category_names = []
        for app_name in index.get('packages') or []:
            cat_name = index.get_package (app_name, 'software').get('category')
            if cat_name and not cat_name in category_names:
                category_names.append (cat_name)

        # Build Content
        cat_list = CTK.List()
        for cat_name in category_names:
            link = CTK.Link ('%s/%s' %(URL_CATEGORY, cat_name), CTK.RawHTML(cat_name))
            cat_list += link

        # Layout
        self += CTK.RawHTML ('<h3>%s</h3>' %(_('Categories')))
        self += cat_list


class Category:
    def __call__ (self):
        index = Distro.Index()
        page = Page_Market(_('Category'))

        # Parse URL
        tmp = re.findall ('^%s/([\w ]+)$'%(URL_CATEGORY), CTK.request.url)
        if not tmp:
            page += CTK.RawHTML ('<h2>%s</h2>' %(_("Empty Category")))
            return page.Render()

        self.category_name = tmp[0]

        # Menu
        menu = Menu([CTK.Link(URL_MAIN, CTK.RawHTML (_('Market Home')))])
        menu += "%s: %s" %(_('Category'), self.category_name)
        page.mainarea += menu

        # Add apps
        pags = CTK.Paginator('category-results', items_per_page=10)

        box = CTK.Box ({'class': 'cherokee-market-category'})
        box += pags
        page.mainarea += box

        for app_name in index.get('packages'):
            app = index.get_package(app_name, 'software')
            cat = app.get('category')
            if cat == self.category_name:
                pags += RenderApp (app)

        # Render
        return page.Render()


CTK.publish ('^%s' %(URL_CATEGORY), Category)
