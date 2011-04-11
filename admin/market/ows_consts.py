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

from consts import *
from configured import *

OWS_STATIC       = 'http://cherokee-market.com'
OWS_APPS         = 'http://www.octality.com/api/v%s/open/market/apps/' %(OWS_API_VERSION)
OWS_APPS_AUTH    = 'http://www.octality.com/api/v%s/market/apps/'      %(OWS_API_VERSION)
OWS_APPS_INSTALL = 'http://www.octality.com/api/v%s/market/install/'   %(OWS_API_VERSION)
OWS_APPS_CENTER  = 'http://www.octality.com/api/v%s/open/appscenter'   %(OWS_API_VERSION)

OWS_DEBUG        = True

URL_MAIN         = '/market'
URL_APP          = '/market/app'
URL_SEARCH       = '/market/search'
URL_SEARCH_APPLY = '/market/search/apply'
URL_CATEGORY     = '/market/category'
URL_REVIEW       = '/market/review'
URL_REPORT       = '/market/report'

REPO_MAIN        = 'http://www.cherokee-project.com/download/distribution'
