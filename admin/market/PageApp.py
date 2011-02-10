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
        def __init__ (self, info, sure_inclusion):
            CTK.Container.__init__ (self)

            # These entries will always be shown
            for entry in sure_inclusion:
                self += SupportBox.Support_Entry (entry, info.get(entry, False))

            # Additional entries
            rest = filter (lambda x: x not in (sure_inclusion), info.keys())
            for entry in rest:
                if info[entry]:
                    self += SupportBox.Support_Entry (entry, True)

    def __init__ (self, info):
        CTK.Box.__init__ (self, {'class': 'market-support-box'})

        OSes    = info['os']
        DBes    = info['db']
        targets = info['target']

        self += CTK.RawHTML ('<h3>%s</h3>' %(_("Deployment methods")))
        self += SupportBox.Support_Entry (_('Virtual Server'), targets.get('host', False))
        self += SupportBox.Support_Entry (_('Directory'),      targets.get('dir',  False))

        self += CTK.RawHTML ('<h3>%s</h3>' %(_("Operating Systems")))
        self += SupportBox.Support_Block (OSes, ('Linux', 'MacOS X', 'Solaris', 'FreeBSD'))

        self += CTK.RawHTML ('<h3>%s</h3>' %(_("Databases")))
        self += SupportBox.Support_Block (DBes, ('MySQL', 'PostgreSQL', 'SQLite', 'Oracle'))


class App:
    cache_app = {}

    def format_func_app (self, info):
        cont = CTK.Container()

        self.menu += "%s %s" %(_('Application'), info['application_name'])
        cont += self.menu

        app_id          = info['application_id']
        currency_symbol = CTK.util.to_utf8 (info['currency_symbol'])

        # Install dialog
        install = InstallDialog (info)
        cont += install

        # Publisher / Author
        by = CTK.Container()
        by += CTK.RawHTML ('%s '%(_('By')))
        if info.get('author_url') and info.get('author_name'):
            by += CTK.LinkWindow (info['author_url'], CTK.RawHTML(info['author_name']))
        else:
            by += CTK.LinkWindow (info['publisher_url'], CTK.RawHTML(info['publisher_name']))

        # Buy / Install button
        if OWS_Login.is_logged():
            if Library.is_appID_in_library (app_id):
                buy = CTK.Button (_("Install"))
            else:
                if info['amount']:
                    buy = CTK.Button ("%s%s %s" %(currency_symbol, info['amount'], _("Buy")))
                else:
                    buy = CTK.Button ("%s, %s" %(_("Free"), _("Install")))
            buy.bind ('click', install.JS_to_show())

        else:
            link = CTK.Link ('#', CTK.RawHTML (_("Sign in")))
            link.bind ('click', self.login_dialog.JS_to_show())

            login_txt = CTK.Box()
            login_txt += link

            if info['amount']:
                buy_button = CTK.Button ("%s%s %s" %(currency_symbol, info['amount'], _("Buy")), {"disabled": True})
                login_txt += CTK.RawHTML (" %s" %(_('to buy')))
            else:
                buy_button = CTK.Button ("%s, %s" %(_("Free"), _("Install")), {"disabled": True})
                login_txt += CTK.RawHTML (" %s" %(_('to install')))

            buy = CTK.Container()
            buy += buy_button
            buy += login_txt

        # Report button
        report = CTK.Container()
        if OWS_Login.is_logged():
            CTK.cfg['tmp!market!report!app_id'] = str(self.text_app_id)

            druid  = CTK.Druid (CTK.RefreshableURL())
            report_dialog = CTK.Dialog ({'title':(_("Report Application")), 'width': 480})
            report_dialog += druid
            druid.bind ('druid_exiting', report_dialog.JS_to_close())

            report_link = CTK.Link (None, CTK.RawHTML(_("Report issue")))
            report_link.bind ('click', report_dialog.JS_to_show() + \
                                       druid.JS_to_goto('"%s"'%(URL_REPORT)))

            report += report_dialog
            report += report_link

        app  = CTK.Box ({'class': 'market-app-desc'})
        app += CTK.Box ({'class': 'market-app-desc-icon'},        CTK.Image({'src': OWS_STATIC + info['icon_big']}))
        app += CTK.Box ({'class': 'market-app-desc-buy'},         buy)
        app += CTK.Box ({'class': 'market-app-desc-title'},       CTK.RawHTML(info['application_name']))
        app += CTK.Box ({'class': 'market-app-desc-version'},     CTK.RawHTML("%s: %s" %(_("Version"), info['version_string'])))
        app += CTK.Box ({'class': 'market-app-desc-url'},         by)
        app += CTK.Box ({'class': 'market-app-desc-category'},    CTK.RawHTML("%s: %s" %(_("Category"), info['category_name'])))
        app += CTK.Box ({'class': 'market-app-desc-short-desc'},  CTK.RawHTML(info['summary']))
        app += CTK.Box ({'class': 'market-app-desc-report'},      report)
        cont += app

        ext_description = CTK.Box ({'class': 'market-app-desc-description'})
        ext_description += CTK.RawHTML(info['description'])
        desc_panel  = CTK.Box ({'class': 'market-app-desc-desc-panel'})
        desc_panel += ext_description
        desc_panel += CTK.Box ({'class': 'market-app-desc-support-box'}, SupportBox(info))

        # Tabs
        tabs = CTK.Tab()

        # Shots
        shots = CTK.CarouselThumbnails()
        shot_entries = info.get('shots', [])

        if shot_entries:
            for s in shot_entries:
                shots += CTK.Image ({'src': "%s/%s" %(OWS_STATIC, s)})
        else:
            shots += CTK.Box ({'id': 'shot-box-empty'}, CTK.RawHTML ('<h2>%s</h2>' %(_("No screenshots"))))

        tabs.Add (_('Screenshots'), shots)
        tabs.Add (_('Description'), desc_panel)
        app += tabs

        # Update the cache
        App.cache_app[str(app_id)] = info

        return cont.Render().toStr()

    def format_func_reviews (self, reviews_info):
        reviews  = reviews_info['reviews']
        app_id   = reviews_info['application_id']
        app_name = reviews_info['application_name']

        cont = CTK.Container()
        cont += CTK.RawHTML ("<h2>%s</h2>" %(_("Reviews")))

        # Render Reviews
        if not reviews:
            cont += CTK.Box ({'class': 'market-no-reviews'}, CTK.RawHTML(_("You can be the first one to review the application")))
        else:
            pags = CTK.Paginator('market-app-reviews', items_per_page=5)
            cont += pags

            for review in reviews:
                d, review_time = review['review_stamp'].value.split('T')
                review_date = "%s-%s-%s" %(d[0:4], d[4:6], d[6:])

                rev  = CTK.Box ({'class': 'market-app-review'})
                rev += CTK.Box ({'class': 'market-app-review-score'},    CTK.StarRating({'selected': review['review_score']}))
                rev += CTK.Box ({'class': 'market-app-review-title'},    CTK.RawHTML(review['review_title']))
                rev += CTK.Box ({'class': 'market-app-review-name'},     CTK.RawHTML(review['first_name'] + ' ' + review['last_name']))
                rev += CTK.Box ({'class': 'market-app-review-stamp'},    CTK.RawHTML(_('on %(date)s at %(time)s' %({'date':review_date, 'time': review_time}))))
                rev += CTK.Box ({'class': 'market-app-review-comment'},  CTK.RawHTML(review['review_comment']))
                pags += rev

        # Review
        if OWS_Login.is_logged():
            CTK.cfg['tmp!market!review!app_id']   = str(app_id)
            CTK.cfg['tmp!market!review!app_name'] = app_name

            druid  = CTK.Druid (CTK.RefreshableURL())
            dialog = CTK.Dialog ({'title':"%s: %s" %(_("Review"), app_name), 'width': 480})
            dialog += druid
            druid.bind ('druid_exiting', dialog.JS_to_close() + \
                                         self.xmlrpc_proxy_reviews.JS_to_refresh())

            add_review = CTK.Button(_("Review"), {'id':'button-review'})
            add_review.bind('click', dialog.JS_to_show() + \
                                     druid.JS_to_goto('"%s"'%(URL_REVIEW)))

            cont += dialog
            cont += add_review
        else:
            sign_box = CTK.Box ({'class': 'market-app-review-signin'})

            link = CTK.Link ('#', CTK.RawHTML (_("sign in")))
            link.bind ('click', self.login_dialog.JS_to_show())

            login_txt  = CTK.Box()
            login_txt += CTK.RawHTML ("%s, " %(_('Please')))
            login_txt += link
            login_txt += CTK.RawHTML (" %s" %(_('to review the product')))

            sign_box += login_txt

            cont += sign_box

        return cont.Render().toStr()

    def __call__ (self):
        page = Page_Market_App (_('Application'))

        #Parse URL
        tmp = re.findall ('^%s/(.+)$' %(URL_APP), CTK.request.url)
        if not tmp:
            page += CTK.RawHTML ('<h2>%s</h2>' %(_("Application not found")))
            return page.Render()

        # Menu
        self.menu = Menu([CTK.Link(URL_MAIN, CTK.RawHTML (_('Market Home')))])

        referer = CTK.request.headers.get ('HTTP_REFERER', '')
        ref_search   = re.findall ('%s/(.+)'%(URL_SEARCH),         referer)
        ref_category = re.findall ('%s/(\d+)/(.+)'%(URL_CATEGORY), referer)

        if ref_search:
            self.menu += CTK.Link ('%s/%s'%(URL_SEARCH, ref_search[0]), CTK.RawHTML("%s %s" %(_('Search'), CTK.unescape_html(ref_search[0]))))
        elif ref_category:
            self.menu += CTK.Link ('%s/%s/%s'%(URL_CATEGORY, ref_category[0][0], ref_category[0][1]), CTK.RawHTML("%s %s"%(_('Category'), CTK.unescape_html(ref_category[0][1]))))

        # Login dialog
        self.login_dialog = OWS_Login.LoginDialog()
        self.login_dialog.bind ('submit_success', CTK.JS.ReloadURL())
        page.mainarea += self.login_dialog

        # App info
        self.text_app_id = tmp[0]
        if App.cache_app.has_key(self.text_app_id):
            page.mainarea += CTK.RawHTML (self.format_func_app (App.cache_app[self.text_app_id]))
        else:
            self.xmlrpc_proxy_app = CTK.XMLRPCProxy (name = 'cherokee-market-app',
                                                     xmlrpc_func = lambda: XmlRpcServer(OWS_APPS).get_full_app (self.text_app_id, OWS_Login.login_session),
                                                     format_func = self.format_func_app,
                                                     debug = OWS_DEBUG)
            page.mainarea += self.xmlrpc_proxy_app

        # App reviews
        self.xmlrpc_proxy_reviews = CTK.XMLRPCProxy (name = 'cherokee-market-app-reviews',
                                                     xmlrpc_func = lambda: XmlRpcServer(OWS_APPS).list_app_reviews (self.text_app_id),
                                                     format_func = self.format_func_reviews,
                                                     debug = OWS_DEBUG)
        page.mainarea += self.xmlrpc_proxy_reviews

        return page.Render()


CTK.publish ('^%s' %(URL_APP), App)
