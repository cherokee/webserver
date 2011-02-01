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
        top += CTK.RawHTML ("<h1>%s</h1>"% _('Market'))
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

        self += CTK.Box ({'class': 'market-app-entry-icon'},     CTK.Image({'src': OWS_STATIC + info['icon_small']}))
        self += CTK.Box ({'class': 'market-app-entry-score'},    CTK.StarRating({'selected': info.get('score', -1)}))
        self += CTK.Box ({'class': 'market-app-entry-price'},    PriceTag(info))
        self += CTK.Box ({'class': 'market-app-entry-name'},     CTK.Link ('%s/%s'%(URL_APP, info['application_id']), CTK.RawHTML(info['application_name'])))
        self += CTK.Box ({'class': 'market-app-entry-category'}, CTK.RawHTML(info['category_name']))
        self += CTK.Box ({'class': 'market-app-entry-summary'},  CTK.RawHTML(info['summary']))


class RenderApp_Featured (CTK.Box):
    def __init__ (self, info, props):
        assert type(info) == dict
        CTK.Box.__init__ (self, props.copy())

        self += CTK.Box ({'class': 'market-app-featured-icon'},     CTK.Image({'src': OWS_STATIC + info['icon_big']}))
        self += CTK.Box ({'class': 'market-app-featured-name'},     CTK.Link ('%s/%s'%(URL_APP, info['application_id']), CTK.RawHTML(info['application_name'])))
        self += CTK.Box ({'class': 'market-app-featured-category'}, CTK.RawHTML(info['category_name']))
        self += CTK.Box ({'class': 'market-app-featured-summary'},  CTK.RawHTML(info['summary']))
        self += CTK.Box ({'class': 'market-app-featured-score'},    CTK.StarRating({'selected': info.get('score', -1)}))


class PriceTag (CTK.Box):
    def __init__ (self, info):
        CTK.Box.__init__ (self, {'class': 'market-app-price'})

        if float(info['amount']):
            self += CTK.Box ({'class': 'currency'}, CTK.RawHTML (info['currency_symbol']))
            self += CTK.Box ({'class': 'amount'},   CTK.RawHTML (str(info['amount'])))
            self += CTK.Box ({'class': 'currency'}, CTK.RawHTML (info['currency']))
        else:
            self += CTK.Box ({'class': 'free'},     CTK.RawHTML (_('Free')))


class InstructionBoxBase (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self)

    def choose_instructions (self, instructions, kwargs):
        data   = SystemInfo.get_info()
        system = data.get('system','').lower()
        distro = data.get('linux_distro_id','').lower()
        info   = instructions.get('default')

        # Optional parameters
        bin_path = kwargs.get('bin_path')

        # MacPorts
        if system == 'darwin' and data.get('macports'):
            macports_path = data.get('macports_path')

            # Do not suggest macports if the bin is outside its scope:
            # /usr/local, for instance.
            if ((not bin_path) or
                (macports_path and bin_path.startswith (macports_path))):
                if instructions.has_key('macports'):
                    return instructions['macports']

        # OS specific
        if instructions.has_key(system):
            return instructions[system]

        # Linux distro specific
        if instructions.has_key(distro):
            return instructions[distro]

        # Linux distro generic
        for x in ('red hat', 'redhat', 'centos'):
            if x in distro:
                return instructions.get('yum', info)

        for x in ('fedora',):
            if x in distro:
                return instructions.get('yumfedora', instructions.get('yum', info))

        for x in ('suse',):
            if x in distro:
                return instructions.get('zypper', info)

        for x in ('debian', 'ubuntu', 'knoppix', 'mint'):
            if x in distro:
                return instructions.get('apt', info)

        # Default
        return instructions['default']


class InstructionBox (InstructionBoxBase):
    def __init__ (self, note, instructions, **kwargs):
        InstructionBoxBase.__init__(self)
        assert type(instructions) in (dict, type(None))

        self += CTK.RawHTML ('<p>%s</p>' %(_(note)))

        if instructions:
            info = self.choose_instructions (instructions, kwargs)
            if info:
                notice  = CTK.Notice ()
                notice += CTK.RawHTML ('<p>%s:</p>' %(_('Try the following instructions to install the required software')))
                notice += CTK.RawHTML ('<pre>%s</pre>' %(_(info)))
                self += notice


class InstructionBoxAlternative (InstructionBoxBase):
    def __init__ (self, note, instruction_list, **kwargs):
        InstructionBoxBase.__init__(self)

        assert type(instruction_list) == list

        self += CTK.RawHTML ('<p>%s</p>' %(_(note)))

        if instruction_list:
            lst     = CTK.List()
            for instructions in instruction_list:
                info = self.choose_instructions (instructions, kwargs)
                if info:
                    lst.Add (CTK.RawHTML('<pre>%s</pre>' %(_(info))))

            notice  = CTK.Notice ()
            notice += CTK.RawHTML ('<p>%s:</p>' %(_('Several alternatives can provide the software requirements for the installation. Try at least one of the following instructions to install your preferred option')))
            notice += lst
            self += notice
