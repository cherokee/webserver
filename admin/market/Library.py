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
import time
import Check
import OWS_Login

from consts import *
from ows_consts import *
from configured import *
from XMLServerDigest import XmlRpcServer

EXPIRATION = 10*60 # 10 min


class MyLibrary (CTK.Box):
    cached_str       = None
    cache_expiration = None

    def format_func (self, info):
        cont = CTK.Container()

        # Empty library
        if not info:
            cont += CTK.RawHTML (_("You have not bought any app yet"))

        # Render app list
        for app in info:
            link = CTK.Link ("%s/%s"%(URL_APP, app['application_id']), CTK.RawHTML (app['application_name']))
            cont += CTK.Box ({'class': 'market-mylibrary-icon'}, CTK.Image({'src': OWS_STATIC + app['icon_small']}))
            cont += CTK.Box ({'class': 'market-mylibrary-name'}, link)

        # Cache
        MyLibrary.cached_str       = cont.Render().toStr()
        MyLibrary.cache_expiration = EXPIRATION

        return self.cached_str

    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'market-my-library'})
        self += CTK.RawHTML ('<h3>%s</h3>' %(_('My Library')))

        self += Check.Unfinished_Installs_None()

        if self.cached_str and self.cache_expiration < time.time():
            self += CTK.RawHTML (self.cached_str)
        else:
            self += CTK.XMLRPCProxy (name = 'cherokee-my-library',
                                     xmlrpc_func = lambda: XmlRpcServer(OWS_APPS_AUTH, OWS_Login.login_user, OWS_Login.login_password).get_user_apps(),
                                     format_func = self.format_func,
                                     debug = OWS_DEBUG)

def Invalidate_Cache():
    MyLibrary.cached_str       = None
    MyLibrary.cache_expiration = 0
