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

import os
import CTK
import Page
import OWS_Login

from consts import *
from ows_consts import *
from configured import *

URL_CLEAN_UP_APPLY = '/check/cleanup/apply'


unfinished_installs_cache = None

def Invalidate_Cache():
    global unfinished_installs_cache
    unfinished_installs_cache = None

def check_unfinished_installations():
    # Check the cache
    global unfinished_installs_cache
    if unfinished_installs_cache:
        return unfinished_installs_cache

    # Check all the applications
    unfinished = []

    if not os.path.exists (CHEROKEE_OWS_ROOT) or \
       not os.access (CHEROKEE_OWS_ROOT, os.R_OK):
        return unfinished

    for d in os.listdir (CHEROKEE_OWS_ROOT):
        fd = os.path.join (CHEROKEE_OWS_ROOT, d)

        if not os.path.isdir(fd):
            continue

        finished = os.path.join (fd, 'finished')
        if not os.path.exists (finished):
            unfinished.append (d)

    unfinished_installs_cache = unfinished
    return unfinished


def Clean_up_Apply():
    for app in check_unfinished_installations():
        fp = os.path.join (CHEROKEE_OWS_ROOT, app)
        os.removedirs (fp)

    return CTK.cfg_reply_ajax_ok()


class Unfinished_Installs_None (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self)

        unfinished = check_unfinished_installations()
        if not len(unfinished):
            return

        self += CTK.RawHTML ('%d %s' %(len(unfinished), _("partial installations: ")))

        submit = CTK.Submitter (URL_CLEAN_UP_APPLY)
        submit += CTK.Hidden ('bar', 'foo')
        submit.bind ('submit_success', self.JS_to_hide())

        link = CTK.Link (None, CTK.RawHTML(_('Clean up')))
        link.bind ('click', submit.JS_to_submit())
        self += link
        self += submit


CTK.publish ('^%s$'%(URL_CLEAN_UP_APPLY), Clean_up_Apply)
