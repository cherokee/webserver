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

from configured import *
from util import version_cmp
from XMLServerDigest import XmlRpcServer
from configured import *

DEBUG = True
OWS_RPC = 'http://www.octality.com/api/v%s/open/market/info/' %(OWS_API_VERSION)


def invalidate_caches():
    Index_Block1.cached = None
    Index_Block2.cached = None


#
# Index
#
class Index_Block1 (CTK.Container):
    cached = None

    def format_func (self, raw_cont):
        cont = CTK.util.to_utf8 (raw_cont)
        Index_Block1.cached = cont
        return cont

    def __init__ (self):
        CTK.Container.__init__ (self)

        # Skip the XML-RPC if OWS is inactive
        if not int (CTK.cfg.get_val ("admin!ows!enabled", OWS_ENABLE)):
            return

        # Instance the XML-RPC Proxy object
        if Index_Block1.cached:
            self += CTK.RawHTML (Index_Block1.cached)
        else:
            self += CTK.XMLRPCProxy (name = 'cherokee-index-block1',
                                     xmlrpc_func = lambda: XmlRpcServer(OWS_RPC).get_block_index_1 (CTK.i18n.active_lang, VERSION),
                                     format_func = self.format_func,
                                     props = {'class': 'main-banner'},
                                     debug = DEBUG)


class Index_Block2 (CTK.Container):
    cached = None

    def format_func (self, raw_cont):
        cont = CTK.util.to_utf8 (raw_cont)
        Index_Block2.cached = cont
        return cont

    def __init__ (self):
        CTK.Container.__init__ (self)

        # Skip the XML-RPC if OWS is inactive
        if not int (CTK.cfg.get_val ('admin!ows!enabled', OWS_ENABLE)):
            return

        # Instance the XML-RPC Proxy object
        if Index_Block2.cached:
            self += CTK.RawHTML (Index_Block2.cached)
        else:
            self += CTK.XMLRPCProxy (name = 'cherokee-index-block2',
                                     xmlrpc_func = lambda: XmlRpcServer(OWS_RPC).get_block_index_2 (CTK.i18n.active_lang, VERSION),
                                     format_func = self.format_func,
                                     debug = DEBUG)


#
# Market
#
class Market_Block1 (CTK.XMLRPCProxy):
    def __init__ (self):
        CTK.XMLRPCProxy.__init__ (self, name = 'cherokee-market-block1',
                                  xmlrpc_func = lambda: XmlRpcServer(OWS_RPC).get_block_market_1 (CTK.i18n.active_lang, VERSION),
                                  format_func = lambda x: x,
                                  debug = DEBUG)

class Market_Block2 (CTK.XMLRPCProxy):
    def __init__ (self):
        CTK.XMLRPCProxy.__init__ (self, name = 'cherokee-market-block2',
                                  xmlrpc_func = lambda: XmlRpcServer(OWS_RPC).get_block_market_2 (CTK.i18n.active_lang, VERSION),
                                  format_func = lambda x: x,
                                  debug = DEBUG)
