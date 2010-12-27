# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega
#
# Copyright (C) 2010 Alvaro Lopez Ortega
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
import time

from util import version_cmp
from XMLServerDigest import XmlRpcServer
from configured import *

OWS_RPC    = 'http://www.octality.com/api/v%s/open/cherokee-info/' %(OWS_API_VERSION)
EXPIRATION = 10*60 # 10 min

BETA_TESTER_NOTICE = N_("Thank you for testing a development snapshot (%(VERSION)s). It helps us to create the highest quality product.")


class Latest_Release (CTK.Box):
    cache_info       = None
    cache_expiration = 0

    def __init__ (self):
        CTK.Box.__init__ (self)

        if Latest_Release.cache_info and \
           Latest_Release.cache_expiration > time.time():
            self += CTK.RawHTML (self.cache_info)
        else:
            self += CTK.XMLRPCProxy ('cherokee-latest-release',
                                     xmlrpc_func = XmlRpcServer(OWS_RPC).get_latest,
                                     format_func = self.format,
                                     debug       = True)

    def format (self, response):
        response = CTK.util.to_utf8(response)
        latest   = response['default']

        # Render
        content  = CTK.Box ({'id': 'latest-release-box', 'class': 'sidebar-box'})
        content += CTK.RawHTML('<h2>%s</h2>' % _('Latest Release'))

        if not latest:
            content += CTK.RawHTML(_('Latest version could not be determined at the moment.'))
        else:
            # Compare the release string
            d = version_cmp (latest['version'], VERSION)
            if d == 0:
                content += CTK.RawHTML (_('Cherokee is up to date') + ': %s'%(VERSION))
            elif d == 1:
                content += CTK.RawHTML ('%s v%s. %s v%s.' %(_('You are running Cherokee'),
                                                            VERSION, _('Latest release is '),
                                                            latest['version']))
            else:
                content += CTK.RawHTML (_(BETA_TESTER_NOTICE)%(globals()))

        # Update cache
        Latest_Release.cache_info       = content.Render().toStr()
        Latest_Release.cache_expiration = time.time() + EXPIRATION

        return Latest_Release.cache_info


#
# Deprecated?
#

class Remote_Tweets (CTK.XMLRPCProxy):
    def __init__ (self):
        CTK.XMLRPCProxy.__init__ (self, 'cherokee-tweets',
                                  XmlRpcServer(OWS_RPC).get_tweets,
                                  self.format, debug=True)

    def format (self, response):
        response = CTK.util.to_utf8(response)

        # [{'date':'', 'link':'Link to the tweet', 'tweet':'', 'txt':'tweet without links'},...]
        lst = CTK.List()
        for r in response:
            lst.Add (CTK.RawHTML('%s <span class="tweet-date">%s</span>'% (r['tweet'], r['date'])))

        return lst.Render().toStr()


class Remote_Mailing_List (CTK.XMLRPCProxy):
    def __init__ (self):
        CTK.XMLRPCProxy.__init__ (self, 'cherokee-mailing-list',
                                  XmlRpcServer(OWS_RPC).get_mailing_list,
                                  self.format, debug=True)

    def format (self, response):
        response = CTK.util.to_utf8(response)

        # [{'authors': '', 'date': '', 'hits': d, 'link', '', 'subject': ''},...]
        lst = CTK.List()
        for r in response:
            lst.Add (CTK.RawHTML('<a href="%s">%s</a> (%s)'% (r['link'],r['subject'], r['authors'])))

        return lst.Render().toStr()

class Remote_Commits (CTK.XMLRPCProxy):
    def __init__ (self):
        CTK.XMLRPCProxy.__init__ (self, 'cherokee-commits',
                                  XmlRpcServer(OWS_RPC).get_commits,
                                  self.format, debug=True)

    def format (self, response):
        response = CTK.util.to_utf8(response)

        # [{'date': '', 'log': '', 'revision': 'dddd', 'time': 'hh:mm:ss', 'url': '', 'user': 'taher'},...]
        lst = CTK.List()
        for r in response:
            lst.Add ([CTK.Box({'class': 'revision'}, CTK.RawHTML('<a href="%s">%s</a>' %(r['url'], r['revision']))),
                      CTK.Box({'class': 'log'},      CTK.RawHTML(r['log']))])

        return lst.Render().toStr()
