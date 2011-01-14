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
import time
import OWS_Login

from consts import *
from ows_consts import *
from configured import *
from XMLServerDigest import XmlRpcServer

EXPIRATION = 10*60 # 10 min

# Cache
cached_info      = None
cache_expiration = None

def Invalidate_Cache():
    global cached_info
    global cache_expiration

    cached_info      = None
    cache_expiration = 0


class MyLibrary (CTK.Box):
    def format_func (self, info, from_cache = False):
        cont = CTK.Container()

        # Empty library
        if not info:
            cont += CTK.RawHTML (_("You have not bought any app yet"))

        # Render app list
        for app in info:
            link = CTK.Link ("%s/%s"%(URL_APP, app['application_id']), CTK.RawHTML (app['application_name']))
            itembox = CTK.Box ({'class': 'market-my-library-item'})
            itembox += CTK.Box ({'class': 'market-my-library-icon'}, CTK.Image({'src': OWS_STATIC + app['icon_small']}))
            itembox += CTK.Box ({'class': 'market-my-library-name'}, link)
            cont += itembox

        # Cache
        if not from_cache:
            global cached_info
            global cache_expiration

            cached_info      = info
            cache_expiration = time.time() + EXPIRATION

        return cont.Render().toStr()

    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'market-my-library'})
        self += CTK.RawHTML ('<h3>%s</h3>' %(_('My Library')))

        if cached_info and cache_expiration > time.time():
            self += CTK.RawHTML (self.format_func (cached_info, from_cache=True))
        else:
            self += CTK.XMLRPCProxy (name = 'cherokee-my-library',
                                     xmlrpc_func = lambda: XmlRpcServer(OWS_APPS_AUTH, OWS_Login.login_user, OWS_Login.login_password).get_user_apps(),
                                     format_func = self.format_func,
                                     debug = OWS_DEBUG)

def update_cache():
    global cached_info
    global cache_expiration

    if not cached_info or cache_expiration < time.time():
        xmlrpc = XmlRpcServer (OWS_APPS_AUTH, OWS_Login.login_user, OWS_Login.login_password)
        cached_info = xmlrpc.get_user_apps()
        cache_expiration = time.time() + EXPIRATION


def is_appID_in_library (app_id):
    update_cache()

    if not cached_info:
        return False

    for app in cached_info:
        if int(app['application_id']) == int(app_id):
            return True

    return False
