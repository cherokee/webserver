# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2010 Alvaro Lopez Ortega
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
import Page
import OWS_Login

from consts import *
from ows_consts import *
from configured import *

HELPS = []

JS_SCROLL = """
function resize_cherokee_containers() {
   $('#market-container').height($(window).height() - 106);
}

$(document).ready(function() {
   resize_cherokee_containers();
   $(window).resize(function(){
       resize_cherokee_containers();
   });
});
"""

class Page_Market (Page.Base):
    def __init__ (self, title=None):
        self.mainarea = CTK.Box ({'class': 'market-main-area'})
        self.sidebar  = CTK.Box({'class': 'market-sidebar'})

        if title:
            full_title = '%s: %s' %(_("Cherokee Market"), title)
        else:
            full_title = _("Cherokee Market")

        Page.Base.__init__ (self, full_title, body_id='market', helps=HELPS)

        # Top
        from PageSearch import Search_Widget
        top = CTK.Box({'id': 'top-box'})
        top += CTK.RawHTML ("<h1>%s</h1>"% _('Market'))
        top += Search_Widget()

        # Main

        # Sidebar
        from PageCategory import Categories_Widget
        self.sidebar += Categories_Widget()

        # Container
        container = CTK.Box({'id': 'market-container'})
        container += self.mainarea
        container += self.sidebar

        self += top
        self += container
        self += CTK.RawHTML (js=JS_SCROLL)



class RenderApp (CTK.Box):
    def __init__ (self, info):
        assert type(info) == dict
        CTK.Box.__init__ (self, {'class': 'market-app-entry'})

        self += CTK.Box ({'class': 'market-app-entry-icon'},     CTK.Image({'src': OWS_STATIC + info['icon_small']}))
        self += CTK.Box ({'class': 'market-app-entry-score'},    CTK.StarRating({'selected': info.get('score', -1)}))
        self += CTK.Box ({'class': 'market-app-entry-price'},    PriceTag(info))
        self += CTK.Box ({'class': 'market-app-entry-name'},     CTK.Link ('%s/%s'%(URL_APP, info['application_id']), CTK.RawHTML(info['application_name'])))
        self += CTK.Box ({'class': 'market-app-entry-category'}, CTK.RawHTML(info['category_name']))
        self += CTK.Box ({'class': 'market-app-entry-summary'},  CTK.RawHTML(info['summary']))


class RenderApp_Featured (CTK.Box):
    def __init__ (self, info, props):
        assert type(info) == dict
        CTK.Box.__init__ (self, props.copy())

        self += CTK.Box ({'class': 'market-app-featured-icon'},     CTK.Image({'src': OWS_STATIC + info['icon_big']}))
        self += CTK.Box ({'class': 'market-app-featured-name'},     CTK.Link ('%s/%s'%(URL_APP, info['application_id']), CTK.RawHTML(info['application_name'])))
        self += CTK.Box ({'class': 'market-app-featured-category'}, CTK.RawHTML(info['category_name']))
        self += CTK.Box ({'class': 'market-app-featured-summary'},  CTK.RawHTML(info['summary']))
        self += CTK.Box ({'class': 'market-app-featured-score'},    CTK.StarRating({'selected': info.get('score', -1)}))


class PriceTag (CTK.Box):
    def __init__ (self, info):
        CTK.Box.__init__ (self, {'class': 'market-app-price'})

        if int(info['amount']):
            self += CTK.Box ({'class': 'currency'}, CTK.RawHTML (info['currency_symbol']))
            self += CTK.Box ({'class': 'amount'},   CTK.RawHTML (info['amount']))
            self += CTK.Box ({'class': 'currency'}, CTK.RawHTML (info['currency']))
        else:
            self += CTK.Box ({'class': 'free'}, CTK.RawHTML (_('Free')))
