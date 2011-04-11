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

import os
import CTK
import Page
import OWS_Login
import SystemInfo

from consts import *
from ows_consts import *
from configured import *

JS_SCROLL = """
function resize_cherokee_containers() {
   $('#market-container').height($(window).height() - 106);
}

$(document).ready(function() {
   resize_cherokee_containers();
   $(window).resize(function(){
       resize_cherokee_containers();
   });
});
"""

class Page_Market (Page.Base):
    def __init__ (self, title=None):
        self.mainarea = CTK.Box ({'class': 'market-main-area'})
        self.sidebar  = CTK.Box({'class': 'market-sidebar'})

        if title:
            full_title = '%s: %s' %(_("Cherokee Market"), title)
        else:
            full_title = _("Cherokee Market")

        Page.Base.__init__ (self, full_title, body_id='market')

        # Top
        from PageSearch import Search_Widget
        top = CTK.Box({'id': 'top-box'})
        top += CTK.RawHTML ("<h1>%s</h1>"% _("Apps Center"))
        top += Search_Widget()

        # Sidebar
        from PageCategory import Categories_Widget
        self.sidebar += Categories_Widget()

        # Container
        container = CTK.Box({'id': 'market-container'})
        container += self.mainarea
        container += self.sidebar

        self += top
        self += container
        self += CTK.RawHTML (js=JS_SCROLL)

class Page_Market_App (Page.Base):
    def __init__ (self, title=None):
        self.mainarea = CTK.Box ({'class': 'market-main-area'})
        self.sidebar  = CTK.Box({'class': 'market-sidebar'})

        full_title = '%s: %s' %(_("Cherokee Market"), title)
        Page.Base.__init__ (self, full_title, body_id='market-app')

        # Top
        from PageSearch import Search_Widget
        top = CTK.Box({'id': 'top-box'})
        top += CTK.RawHTML ("<h1>%s</h1>"% _('Market'))
        top += Search_Widget()

        # Container
        container = CTK.Box({'id': 'market-container'})
        container += self.mainarea
        container += self.sidebar

        self += top
        self += container
        self += CTK.RawHTML (js=JS_SCROLL)


class RenderApp (CTK.Box):
    def __init__ (self, info):
        assert type(info) == dict
        CTK.Box.__init__ (self, {'class': 'market-app-entry'})

        repo_url = CTK.cfg.get_val ('admin!ows!repository', REPO_MAIN)
        url_icon_small = os.path.join (repo_url, info['id'], "icons", info['icon_small'])

        self += CTK.Box ({'class': 'market-app-entry-icon'},     CTK.Image({'src': url_icon_small}))
        self += CTK.Box ({'class': 'market-app-entry-score'},    CTK.StarRating({'selected': info.get('score', -1)}))
        # self += CTK.Box ({'class': 'market-app-entry-price'},    PriceTag(info))
        self += CTK.Box ({'class': 'market-app-entry-name'},     CTK.Link ('%s/%s'%(URL_APP, info['id']), CTK.RawHTML(info['name'])))
        self += CTK.Box ({'class': 'market-app-entry-category'}, CTK.RawHTML(info['category']))
        self += CTK.Box ({'class': 'market-app-entry-summary'},  CTK.RawHTML(info['desc_short']))


class PriceTag (CTK.Box):
    def __init__ (self, info):
        CTK.Box.__init__ (self, {'class': 'market-app-price'})

        if float(info['amount']):
            self += CTK.Box ({'class': 'currency'}, CTK.RawHTML (info['currency_symbol']))
            self += CTK.Box ({'class': 'amount'},   CTK.RawHTML (str(info['amount'])))
            self += CTK.Box ({'class': 'currency'}, CTK.RawHTML (info['currency']))
        else:
            self += CTK.Box ({'class': 'free'},     CTK.RawHTML (_('Free')))


class InstructionBox (CTK.Box):
    def __init__ (self, note, instructions=None, **kwargs):
        CTK.Box.__init__ (self)
        assert type(instructions) in (dict, list, type(None))

        self += CTK.RawHTML ('<p>%s</p>' %(_(note)))

        if instructions == None:
            return

        elif type(instructions) == dict:
            info = self.choose_instructions (instructions, kwargs)

        elif type(instructions) == list: # This was the InstructionBoxAlternative behavior
            info = []
            for inst_dict in instructions:
                info += self.choose_instructions (inst_dict, kwargs)

        self.__add (info)


    def __add (self, info):
        if not info:
            return
        elif len(info) == 1:
            self.__add_one (info[0])
        else:
            self.__add_many (info)


    def __add_one (self, info):
        notice  = CTK.Notice ()
        notice += CTK.RawHTML ('<p>%s:</p>' %(_('Try the following instructions to install the required software')))
        notice += CTK.RawHTML ('<pre>%s</pre>' %(_(info)))
        self += notice


    def __add_many (self, info_list):
        notice  = CTK.Notice ()
        notice += CTK.RawHTML ('<p>%s:</p>' %(_('Several alternatives can provide the software requirements for the installation. Try at least one of the following instructions to install your preferred option')))

        for info in info_list:
            lst = CTK.List()
            lst.Add (CTK.RawHTML('<pre>%s</pre>' %(_(info))))
            notice += lst
            if info != info_list[-1]:
                notice += CTK.RawHTML('<em>%s</em>' %(_('or')))

        self += notice


    def choose_instructions (self, instructions, kwargs={}):
        data    = SystemInfo.get_info()
        system  = data.get('system','').lower()
        distro  = data.get('linux_distro_id','').lower()
        default = instructions.get('default')

        # Optional parameters
        bin_path = kwargs.get('bin_path')

        # MacPorts
        if system == 'darwin' and data.get('macports'):
            macports_path = data.get('macports_path')

            # Do not suggest macports if the bin is outside its scope:
            # /usr/local, for instance.
            if ((not bin_path) or
                (macports_path and bin_path.startswith (macports_path))):
                return [instructions.get('macports', default)]

        # FreeBSD: pkg_add *and* ports
        if system == 'freebsd':
            choices = []

            if instructions.has_key('freebsd_pkg'):
                choices += [instructions.get('freebsd_pkg')]

            if data.get('freebsd_ports_path') and instructions.has_key('freebsd_ports'):
                choices += [instructions.get('freebsd_ports')]

            if choices:
                return choices

        # Solaris' IPS
        if system.startswith ('sunos'):
            if os.path.exists ("/usr/bin/pkg"):
                return [instructions.get ('ips', default)]

        # OS specific
        if instructions.has_key(system):
            return [instructions.get (system, default)]

        # Linux distro specific
        if instructions.has_key(distro):
            return [instructions.get (distro, default)]

        # Linux distro generic
        for x in ('red hat', 'redhat', 'centos'):
            if x in distro:
                return [instructions.get ('yum', default)]

        for x in ('fedora',):
            if x in distro:
                return [instructions.get ('yumfedora', instructions.get('yum', default))]

        for x in ('suse',):
            if x in distro:
                return [instructions.get ('zypper', default)]

        for x in ('debian', 'ubuntu', 'knoppix', 'mint'):
            if x in distro:
                return [instructions.get ('apt', default)]

        # Default
        return [default]
