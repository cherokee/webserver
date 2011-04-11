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
import Distro

from Util import *
from consts import *
from ows_consts import *
from configured import *
from XMLServerDigest import XmlRpcServer


class RenderApp_Featured (CTK.Box):
    def __init__ (self, info, props):
        assert type(info) == dict
        CTK.Box.__init__ (self, props.copy())

        url_icon_big = '%s/%s/icons/%s' %(REPO_MAIN, info['id'], info['icon_big'])

        self += CTK.Box ({'class': 'market-app-featured-icon'},     CTK.Image({'src': url_icon_big}))
        self += CTK.Box ({'class': 'market-app-featured-name'},     CTK.Link ('%s/%s'%(URL_APP, info['id']), CTK.RawHTML(info['name'])))
        self += CTK.Box ({'class': 'market-app-featured-category'}, CTK.RawHTML(info['category']))
        self += CTK.Box ({'class': 'market-app-featured-summary'},  CTK.RawHTML(info['desc_short']))
        self += CTK.Box ({'class': 'market-app-featured-score'},    CTK.StarRating({'selected': info.get('score', -1)}))


class FeaturedBox (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'market-featured-box'})
        index = Distro.Index()

        # List
        app_list    = CTK.List ({'class': 'market-featured-box-list'})
        app_content = CTK.Box  ({'class': 'market-featured-box-content'})

        self += app_content
        self += app_list

        app_content += CTK.RawHTML("<h2>%s</h2>" %(_('Featured Applications')))

        # Build the list
        featured_apps = list(index.get('featured_apps') or [])
        for app_id in featured_apps:
            app = index.get_package (app_id, 'software')

            # Content entry
            props = {'class': 'market-app-featured',
                     'id':    'market-app-featured-%s' %(app_id)}

            if featured_apps.index(app_id) != 0:
                props['style'] = 'display:none'

            app_content += RenderApp_Featured (app, props)

            # List entry
            url_icon_small = '%s/%s/icons/%s' %(REPO_MAIN, app['id'], app['icon_small'])

            list_cont  = CTK.Box ({'class': 'featured-list-entry'})
            list_cont += CTK.Box ({'class': 'featured-list-icon'},     CTK.Image({'src': url_icon_small}))
            list_cont += CTK.Box ({'class': 'featured-list-name'},     CTK.RawHTML(app['name']))
            list_cont += CTK.Box ({'class': 'featured-list-category'}, CTK.RawHTML(app['category']))
            if featured_apps.index(app_id) != 0:
                app_list.Add(list_cont, {'id': 'featured-list-%s' %(app_id)})
            else:
                app_list.Add(list_cont, {'id': 'featured-list-%s' %(app_id), 'class': 'selected'})


            list_cont.bind ('click',
                            "var list = $('.market-featured-box-list');" +
                            "list.find('li.selected').removeClass('selected');"     +
                            "list.find('#featured-list-%s').addClass('selected');"  %(app_id) +
                            "var content = $('.market-featured-box-content');" +
                            "content.find('.market-app-featured').hide();"     +
                            "content.find('#market-app-featured-%s').show();"  %(app_id))



class TopApps (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'market-top-box'})
        index = Distro.Index()

        for app_name in index.get('top_apps') or []:
            app = index.get_package(app_name, 'software')
            self += RenderApp (app)


class RepoError (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'market-repo-error'})

        repo_url = CTK.cfg.get_val ('admin!ows!repository', REPO_MAIN)

        self += CTK.RawHTML ('<h2>%s</h2>'%(_("Could not access the repository")))
        self += CTK.RawHTML ('<p>%s: ' %(_("Cherokee could not reach the remote package repository")))
        self += CTK.LinkWindow (repo_url, CTK.RawHTML(repo_url))
        self += CTK.RawHTML ('.</p>')


class Main:
    def __call__ (self):
        # Ensure OWS is enabled
        if not int(CTK.cfg.get_val("admin!ows!enabled", OWS_ENABLE)):
            return CTK.HTTP_Redir('/')

        page = Page_Market()

        # Check repository is accessible
        index = Distro.Index()
        if index.error:
            page.mainarea += RepoError()
            return page.Render()

        # Featured
        page.mainarea += FeaturedBox()

        # Top
        page.mainarea += CTK.RawHTML("<h2>%s</h2>" %(_('Top Applications')))
        page.mainarea += TopApps()

        # Maintanance
        if Maintenance.does_it_need_maintenance():
            refresh_maintenance = CTK.Refreshable({'id': 'market_maintenance'})
            refresh_maintenance.register (lambda: Maintenance.Maintenance_Box(refresh_maintenance).Render())
            page.sidebar += refresh_maintenance

        return page.Render()


CTK.publish ('^%s$'%(URL_MAIN), Main)
