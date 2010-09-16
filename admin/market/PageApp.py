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


class App:
    cache_app = {}

    def format_func_app (self, info):
        cont = CTK.Container()

        self.menu += "%s %s" %(_('Application'), info['application_name'])
        cont += self.menu

        app_id = info['application_id']

        # Install dialog
        install = InstallDialog (info)
        cont += install

        # Content
        by = CTK.Container()
        by += CTK.RawHTML ('%s '%(_('By')))
        if info.get('author_url') and info.get('author_name'):
            by += CTK.LinkWindow (info['author_url'], CTK.RawHTML(info['author_name']))
        else:
            by += CTK.LinkWindow (info['publisher_url'], CTK.RawHTML(info['publisher_name']))

        price = PriceTag(info)
        if OWS_Login.is_logged():
            if Library.is_appID_in_library (app_id):
                buy = CTK.Button (_("Install"))
            else:
                buy = CTK.Button (_("Buy"))
            buy.bind ('click', install.JS_to_show())
            price += buy
        else:
            link = CTK.Link ('#', CTK.RawHTML (_("Sign in")))
            link.bind ('click', self.login_dialog.JS_to_show())

            login_txt = CTK.Container()
            login_txt += link
            login_txt += CTK.RawHTML (" %s" %(_('to buy')))

            buy = CTK.Button (_("Buy"), {"disabled": True})
            price += buy
            price += login_txt

        app  = CTK.Box ({'class': 'market-app-desc'})
        app += CTK.Box ({'class': 'market-app-desc-icon'},        CTK.Image({'src': OWS_STATIC + info['icon_big']}))
        app += CTK.Box ({'class': 'market-app-desc-price'},       price)
        app += CTK.Box ({'class': 'market-app-desc-title'},       CTK.RawHTML(info['application_name']))
        app += CTK.Box ({'class': 'market-app-desc-version'},     CTK.RawHTML("%s: %s" %(_("Version"), info['version_string'])))
        app += CTK.Box ({'class': 'market-app-desc-url'},         by)
        app += CTK.Box ({'class': 'market-app-desc-category'},    CTK.RawHTML("%s: %s" %(_("Category"), info['category_name'])))
        app += CTK.Box ({'class': 'market-app-desc-short-desc'},  CTK.RawHTML(info['summary']))
        cont += app

        # Description
        title1 = CTK.RawHTML ("☟ Read more…")
        title2 = CTK.RawHTML ("☝ Read less…")

        collapsible = CTK.Collapsible ((title1, title2), True)
        collapsible += CTK.Box ({'class': 'market-app-desc-description'}, CTK.RawHTML(info['description']))
        app += collapsible

        # Shots
        shot_entries = info.get('shots', [])
        if shot_entries:
            shots = CTK.Carousel()
            for s in shot_entries:
                shots += CTK.Image ({'src': "%s/%s" %(OWS_STATIC, s)})
            app += shots

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
            cont += CTK.RawHTML (_("You can be the first one to review the application"))
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
                rev += CTK.Box ({'class': 'market-app-review-stamp'},    CTK.RawHTML(_('on %s at %s' %(review_date, review_time))))
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

            signin = CTK.Button(_("Sign in"))
            signin.bind ('click', self.login_dialog.JS_to_show())

            sign_box += CTK.RawHTML (_("Please, sign in to review the product."))
            sign_box += signin
            cont += sign_box

        return cont.Render().toStr()

    def __call__ (self):
        page = Page_Market(_('Application'))

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


CTK.publish ('^%s' %(URL_APP),          App)
