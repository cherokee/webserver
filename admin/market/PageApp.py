# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#      Taher Shihadeh <taher@unixwars.com>
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
import Distro
import OWS_Login

from Util import *
from Menu import *
from Install import *

from consts import *
from ows_consts import *
from configured import *

from XMLServerDigest import XmlRpcServer

WEBDIR = N_('Web Directory')
VHOST  = N_('Virtual Server')


class SupportBox (CTK.Box):
    class Support_Entry (CTK.Box):
        def __init__ (self, name, supported):
            CTK.Box.__init__ (self, {'class': 'market-support-entry'})
            if supported:
                self.props['title']  = _('Supported')
                self.props['class'] += ' supported'
            else:
                self.props['title']  = _('Unsupported')
                self.props['class'] += ' unsupported'

            self += CTK.RawHTML (name)

    class Support_Block (CTK.Container):
        REPLACEMENTS = {
            'macosx':  'MacOS X',
            'sqlite3': 'SQLite',
        }
        def __init__ (self, info, sure_inclusion):
            CTK.Container.__init__ (self)

            info       = list(info)
            info_lower = [x.lower() for x in info]
            sure_lower = [x.lower() for x in sure_inclusion]

            # Initial replacements
            for rep in self.REPLACEMENTS:
                if rep in info_lower:
                    info[info.index(rep)]             = self.REPLACEMENTS[rep]
                    info_lower[info_lower.index(rep)] = self.REPLACEMENTS[rep].lower()

            # These entries will always be shown
            for entry in sure_inclusion:
                self += SupportBox.Support_Entry (entry, entry.lower() in info_lower)

            # Additional entries
            for entry in info:
                if entry.lower() in sure_lower:
                    continue
                if entry.lower() in info_lower:
                    self += SupportBox.Support_Entry (entry, True)

    def __init__ (self, app_name):
        CTK.Box.__init__ (self, {'class': 'market-support-box'})

        index = Distro.Index()
        inst = index.get_package (app_name, 'installation')

        OSes  = inst['OS']
        DBes  = inst['DB']
        modes = inst['modes']

        self += CTK.RawHTML ('<h3>%s</h3>' %(_("Deployment methods")))
        self += SupportBox.Support_Entry (_('Virtual Server'), 'vserver' in modes)
        self += SupportBox.Support_Entry (_('Directory'),      'dir'     in modes)

        self += CTK.RawHTML ('<h3>%s</h3>' %(_("Operating Systems")))
        self += SupportBox.Support_Block (OSes, ('Linux', 'MacOS X', 'Solaris', 'FreeBSD'))

        self += CTK.RawHTML ('<h3>%s</h3>' %(_("Databases")))
        self += SupportBox.Support_Block (DBes, ('MySQL', 'PostgreSQL', 'SQLite', 'Oracle'))



class AppInfo (CTK.Box):
    def __init__ (self, app_name):
        CTK.Box.__init__ (self, {'class': 'cherokee-market-app'})

        index = Distro.Index()

        app        = index.get_package (app_name, 'software')
        maintainer = index.get_package (app_name, 'maintainer')

        # Install dialog
        install = InstallDialog (app_name)

        # Author
        by = CTK.Container()
        by += CTK.RawHTML ('%s '%(_('By')))
        by += CTK.LinkWindow (app['URL'], CTK.RawHTML(app['author']))

        install_button = CTK.Button (_("Install"))
        install_button.bind ('click', install.JS_to_show())

        # Report button
        druid = CTK.Druid (CTK.RefreshableURL())
        report_dialog = CTK.Dialog ({'title':(_("Report Application")), 'width': 480})
        report_dialog += druid
        druid.bind ('druid_exiting', report_dialog.JS_to_close())

        report_link = CTK.Link (None, CTK.RawHTML(_("Report issue")))
        report_link.bind ('click', report_dialog.JS_to_show() + \
                                   druid.JS_to_goto('"%s/%s"'%(URL_REPORT, app_name)))

        report = CTK.Container()
        report += report_dialog
        report += report_link

        # Info
        repo_url = CTK.cfg.get_val ('admin!ows!repository', REPO_MAIN)
        url_icon_big = os.path.join (repo_url, app['id'], "icons", app['icon_big'])

        appw  = CTK.Box ({'class': 'market-app-desc'})
        appw += CTK.Box ({'class': 'market-app-desc-icon'},        CTK.Image({'src': url_icon_big}))
        appw += CTK.Box ({'class': 'market-app-desc-buy'},         install_button)
        appw += CTK.Box ({'class': 'market-app-desc-title'},       CTK.RawHTML(app['name']))
        appw += CTK.Box ({'class': 'market-app-desc-version'},     CTK.RawHTML("%s: %s" %(_("Version"), app['version'])))
        appw += CTK.Box ({'class': 'market-app-desc-url'},         by)
        appw += CTK.Box ({'class': 'market-app-desc-packager'},    CTK.RawHTML("%s: %s" %(_("Packager"), maintainer['name'] or _("Orphan"))))
        appw += CTK.Box ({'class': 'market-app-desc-category'},    CTK.RawHTML("%s: %s" %(_("Category"), app['category'])))
        appw += CTK.Box ({'class': 'market-app-desc-short-desc'},  CTK.RawHTML(app['desc_short']))
        appw += CTK.Box ({'class': 'market-app-desc-report'},      report)

        # Support
        ext_description = CTK.Box ({'class': 'market-app-desc-description'})
        ext_description += CTK.RawHTML(app['desc_long'])
        desc_panel  = CTK.Box ({'class': 'market-app-desc-desc-panel'})
        desc_panel += ext_description
        desc_panel += CTK.Box ({'class': 'market-app-desc-support-box'}, SupportBox(app_name))

        # Shots
        shots = CTK.CarouselThumbnails()
        shot_entries = app.get('screenshots', [])

        if shot_entries:
            for s in shot_entries:
                shots += CTK.Image ({'src': os.path.join (repo_url, app_name, "screenshots", s)})
        else:
            shots += CTK.Box ({'id': 'shot-box-empty'}, CTK.RawHTML ('<h2>%s</h2>' %(_("No screenshots"))))

        # Tabs
        tabs = CTK.Tab()
        tabs.Add (_('Screenshots'), shots)
        tabs.Add (_('Description'), desc_panel)

        # GUI Layout
        self += appw
        self += tabs
        self += install


class App:
    def __call__ (self):
        page = Page_Market_App (_('Application'))

        # Figure app
        tmp = re.findall ('^%s/(.+)$' %(URL_APP), CTK.request.url)
        if not tmp:
            page += CTK.RawHTML ('<h2>%s</h2>' %(_("Application not found")))
            return page.Render()

        app_name = tmp[0]

        # Menu
        self.menu = Menu([CTK.Link(URL_MAIN, CTK.RawHTML (_('Market Home')))])
        page.mainarea += self.menu

        referer = CTK.request.headers.get ('HTTP_REFERER', '')
        ref_search   = re.findall ('%s/(.+)'%(URL_SEARCH),       referer)
        ref_category = re.findall ('%s/([\w ]+)'%(URL_CATEGORY), referer)

        if ref_search:
            self.menu += CTK.Link ('%s/%s'%(URL_SEARCH, ref_search[0]), CTK.RawHTML("%s %s" %(_('Search'), CTK.unescape_html(ref_search[0]))))
        elif ref_category:
            self.menu += CTK.Link ('%s/%s'%(URL_CATEGORY, ref_category[0]), CTK.RawHTML("%s %s"%(_('Category'), CTK.unescape_html(ref_category[0]))))

        # App Info
        page.mainarea += AppInfo (app_name)

        # Final render
        return page.Render()


CTK.publish ('^%s' %(URL_APP), App)
