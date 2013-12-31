# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
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
import Cherokee
import validations

from util import *

URL_BASE  = r'/source'
URL_APPLY = r'/source/apply'

HELPS = [('config_virtual_servers', N_("Virtual Servers"))]

def commit():
    for k in CTK.post:
        CTK.cfg[k] = CTK.post[k]
    return {'ret': 'ok'}

class Panel (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'id': 'sources-panel'})

class Render:
    def __call__ (self):
        title = _('Information Sources')

        page = Page.Base (title, helps=HELPS)
        page += CTK.RawHTML ("<h1>%s</h1>" % (title))
        page += Panel()
        return page.Render()


CTK.publish (URL_BASE,  Render)
CTK.publish (URL_APPLY, commit, method="POST")
