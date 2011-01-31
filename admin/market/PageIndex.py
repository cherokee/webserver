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

import CTK
import Library
import OWS_Login
import Maintenance

from Util import *
from consts import *
from ows_consts import *
from configured import *
from XMLServerDigest import XmlRpcServer


class FeaturedBox (CTK.Box):
    cache = []

    def format_func (self, info):
        cont = CTK.Container()

        # List
        app_list    = CTK.List ({'class': 'market-featured-box-list'})
        app_content = CTK.Box  ({'class': 'market-featured-box-content'})

        cont += app_content
        cont += app_list

        app_content += CTK.RawHTML("<h2>%s</h2>" %(_('Featured Applications')))

        # Build the list
        for app in info:
            # Content entry
            props = {'class': 'market-app-featured',
                     'id':    'market-app-featured-%s' %(app['application_id'])}

            if info.index(app) != 0:
                props['style'] = 'display:none'

            app_content += RenderApp_Featured (app, props)

            # List entry
            list_cont  = CTK.Box ({'class': 'featured-list-entry'})
            list_cont += CTK.Box ({'class': 'featured-list-icon'},     CTK.Image({'src': OWS_STATIC + app['icon_small']}))
            list_cont += CTK.Box ({'class': 'featured-list-name'},     CTK.RawHTML(app['application_name']))
            list_cont += CTK.Box ({'class': 'featured-list-category'}, CTK.RawHTML(app['category_name']))
            if info.index(app) != 0:
                app_list.Add(list_cont, {'id': 'featured-list-%s' %(app['application_id'])})
            else:
                app_list.Add(list_cont, {'id': 'featured-list-%s' %(app['application_id']), 'class': 'selected'})


            list_cont.bind ('click',
                            "var list = $('.market-featured-box-list');" +
                            "list.find('li.selected').removeClass('selected');"     +
                            "list.find('#featured-list-%s').addClass('selected');"  %(app['application_id']) +
                            "var content = $('.market-featured-box-content');" +
                            "content.find('.market-app-featured').hide();"     +
                            "content.find('#market-app-featured-%s').show();"  %(app['application_id']))

        # Render and cache
        render = cont.Render().toStr()
        if len(info) > 2 and not self.cache:
            FeaturedBox.cache = info

        return render

    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'market-featured-box'})

        if self.cache:
            # Cache hit
            self += CTK.RawHTML (self.format_func(self.cache))
        else:
            # Cache miss
            self += CTK.XMLRPCProxy(name = 'cherokee-featured-box',
                                    xmlrpc_func = lambda: XmlRpcServer(OWS_APPS).get_featured_apps (OWS_Login.login_session),
                                    format_func = self.format_func,
                                    debug = OWS_DEBUG)


class TopApps (CTK.Container):
    cache = {}

    def format_func (self, info):
        box = CTK.Box ({'class': 'market-top-box'})
        for app in info:
            box += RenderApp (app)

        render = box.Render().toStr()
        if len(info) > 2:
            self.cache[self.klass] = render

        return render

    def __init__ (self, klass):
        CTK.Container.__init__ (self)
        self.klass = klass

        if self.cache.has_key(klass):
            # Cache hit
            self += CTK.RawHTML (self.cache[klass])
        else:
            # Cache miss
            self += CTK.XMLRPCProxy(name = 'cherokee-market-category-%s'%(klass),
                                    xmlrpc_func = lambda: XmlRpcServer(OWS_APPS).get_top_apps (klass, OWS_Login.login_session),
                                    format_func = self.format_func,
                                    debug = OWS_DEBUG)


class Main:
    def __call__ (self):
        # Ensure OWS is enabled
        if not int(CTK.cfg.get_val("admin!ows!enabled", OWS_ENABLE)):
            return CTK.HTTP_Redir('/')

        page = Page_Market()

        # Featured
        page.mainarea += FeaturedBox()

        # Top
        page.mainarea += CTK.RawHTML("<h2>%s</h2>" %(_('Top Applications')))
        tabs = CTK.Tab()
        tabs.Add (_('Paid'),    TopApps('paid'))
        tabs.Add (_('Free'),    TopApps('free'))
        tabs.Add (_('Popular'), TopApps('any'))
        page.mainarea += tabs

        # My Library
        if OWS_Login.is_logged():
            page.sidebar += Library.MyLibrary()

        # Maintanance
        if Maintenance.does_it_need_maintenance():
            refresh_maintenance = CTK.Refreshable({'id': 'market_maintenance'})
            refresh_maintenance.register (lambda: Maintenance.Maintenance_Box(refresh_maintenance).Render())
            page.sidebar += refresh_maintenance

        return page.Render()


CTK.publish ('^%s$'%(URL_MAIN), Main)
