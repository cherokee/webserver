# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#      Taher Shihadeh <taher@unixwars.com>
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
import PageError
import SystemStats
import SystemStatsWidgets
import About

import os
import time
import urllib
import urllib2
import re

from util import *
from consts import *
from configured import *

# Links
LINK_BUGTRACKER = 'http://bugs.cherokee-project.com/'
LINK_TWITTER    = 'http://twitter.com/webserver'
LINK_FACEBOOK   = 'http://www.facebook.com/cherokee.project'
LINK_GOOGLEPLUS = 'https://plus.google.com/u/1/communities/109478817835447552345'
LINK_GITHUB     = 'https://github.com/cherokee/webserver'
LINK_DOWNLOAD   = 'http://www.cherokee-project.com/download/'
LINK_LIST       = 'https://groups.google.com/forum/#!forum/cherokee-http'
LINK_IRC        = 'irc://irc.freenode.net/cherokee'
LINK_HELP       = '/help/basics.html'
LINK_CHEROKEE   = 'http://www.cherokee-project.com/'
LINK_LINKEDIN   = 'http://www.linkedin.com/groups?gid=1819726'

# Server is..
RUNNING_NOTICE      = N_('Server is Running')
STOPPED_NOTICE      = N_('Server is not Running')

# Help entries
HELPS = [('config_status', N_("Status"))]

JS_SCROLL = """
function resize_cherokee_containers() {
   $('#home-container').height($(window).height() - 106);
}

$(document).ready(function() {
   resize_cherokee_containers();
   $(window).resize(function(){
       resize_cherokee_containers();
   });
});
"""


def Halt():
    # This function halts Cherokee-*admin*
    Cherokee.admin.halt()
    return {'ret': 'ok'}


def Launch():
    if not Cherokee.server.is_alive():
        error = Cherokee.server.launch()
        if error:
            page_error = PageError.PageErrorLaunch (error)
            return page_error.Render()

    return CTK.HTTP_Redir('/')

def Stop():
    Cherokee.pid.refresh()
    Cherokee.server.stop()
    return CTK.HTTP_Redir('/')


class ServerInfo (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'id': 'server-section', 'class': 'infosection'})

        infotable = CTK.Table({'class': 'info-table'})
        infotable.set_header (column=True, num=1)

        is_alive = Cherokee.server.is_alive()
        entry = lambda title, string: [CTK.RawHTML (title), CTK.RawHTML(str(string))]

        if is_alive:
            button = CTK.Button(_('Stop Server'), {'id': 'launch-button', 'class': 'button-stop'})
            button.bind ('click', CTK.JS.GotoURL('/stop'))
            infotable += [CTK.RawHTML(_(RUNNING_NOTICE)), button]
        else:
            button = CTK.Button(_('Start Server'), {'id': 'launch-button', 'class': 'button-start'})
            button.bind ('click', CTK.JS.GotoURL('/launch'))
            infotable += [CTK.RawHTML(_(STOPPED_NOTICE)), button]

        sys_stats = SystemStats.get_system_stats()
        infotable += entry(_('Hostname'), sys_stats.hostname)
        if CTK.cfg.file:
            cfg_file = '<span title="%s: %s">%s</span>' % (_('Modified'), self._get_cfg_ctime(), CTK.cfg.file)
        else:
            cfg_file = _('Not found')
        infotable += entry(_("Config File"), cfg_file)

        box = CTK.Box()
        box += infotable

        table = CTK.Table()
        table.set_header (column=True, num=1)
        table += [CTK.RawHTML (_('Server Information')), box]
        self += table

    def _get_cfg_ctime (self):
        info = os.stat(CTK.cfg.file)
        return time.ctime(info.st_ctime)

class CPUInfo (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'id': 'cpu-section', 'class': 'infosection'})
        table = CTK.Table()
        table.set_header (column=True, num=1)
        table += [CTK.RawHTML (_('Processors')), SystemStatsWidgets.CPU_Info()]
        table += [CTK.RawHTML (''), SystemStatsWidgets.CPU_Meter()]
        self += table

class MemoryInfo (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'id': 'ram-section', 'class': 'infosection'})

        meter = SystemStatsWidgets.Memory_Meter()

        table = CTK.Table()
        table.set_header (column=True, num=1)
        table += [CTK.RawHTML (_('Memory')), meter]
        self += table


def language_set (langs):
    languages = [langs]
    try:
        CTK.i18n.install_translation('cherokee', LOCALEDIR, languages)
    except:
        CTK.util.print_exception()
        return True

def Lang_Apply():
    # Sanity check
    lang = CTK.post.get_val('lang')
    if not lang:
        return {'ret': 'error', 'errors': {'lang': 'Cannot be empty'}}

    language_set (lang)
    CTK.cfg['admin!lang'] = lang

    return {'ret': 'ok', 'redirect': '/'}


class LanguageSelector (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'id': 'language-selector'})
        languages = [('', _('Choose'))] + trans_options(AVAILABLE_LANGUAGES)

        submit = CTK.Submitter('/lang/apply')
        submit.id = 'language-list'
        # TODO: Maybe it's better to show selected lang and ommit 'Language' label.
        submit += CTK.Combobox ({'name': 'lang'}, languages)

        self += CTK.RawHTML('%s: ' %(_('Language')))
        self += submit


class SupportBox (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'id': 'support-box', 'class': 'sidebar-box'})

        qlist = CTK.List ()
        self += CTK.RawHTML('<h2>%s</h2>' % _('Support'))
        self += qlist

        # Help
        link = CTK.LinkWindow (LINK_HELP, CTK.RawHTML (_('Getting started')))
        qlist += link

        # Mailing List
        link = CTK.LinkWindow (LINK_LIST, CTK.RawHTML (_('Visit the mailing list')))
        qlist += link

        # Bug report
        link = CTK.LinkWindow (LINK_BUGTRACKER, CTK.RawHTML (_('Report a bug')))
        qlist += link

        # About..
        dialog = CTK.DialogProxyLazy (About.URL_ABOUT_CONTENT, {'title': _("About Cherokee"), 'width': 600})
        dialog.AddButton (_('Close'), 'close')
        self += dialog

        link = CTK.Link ('#', CTK.RawHTML (_('About Cherokee')))
        link.bind ('click', dialog.JS_to_show())
        qlist += link

class CommunityBar (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'id': 'community-bar'})

        slist = CTK.List ()
        self += CTK.RawHTML('<h3>%s</h3>' % _('Join the Cherokee Community:'))
        self += slist

        slist += CTK.LinkWindow (LINK_CHEROKEE,   CTK.Image({'src': '/static/images/other/web.png',        'title': _('Visit the Cherokee Project Website')}))
        slist += CTK.LinkWindow (LINK_TWITTER,    CTK.Image({'src': '/static/images/other/twitter.png',    'title': _('Follow us on Twitter')}))
        slist += CTK.LinkWindow (LINK_FACEBOOK,   CTK.Image({'src': '/static/images/other/facebook.png',   'title': _('Join us on Facebook')}))
        slist += CTK.LinkWindow (LINK_GOOGLEPLUS, CTK.Image({'src': '/static/images/other/googleplus.png', 'title': _('Join us on Google+')}))
        slist += CTK.LinkWindow (LINK_GITHUB,     CTK.Image({'src': '/static/images/other/github.png',     'title': _('Fork us on Github')}))
        slist += CTK.LinkWindow (LINK_LINKEDIN,   CTK.Image({'src': '/static/images/other/linkedin.png',   'title': _('Become a member of Cherokee group on LinkedIn')}))
        slist += CTK.LinkWindow (LINK_IRC,        CTK.Image({'src': '/static/images/other/irc.png',        'title': _('Chat with us at irc.freenode.net')}))

class HaltAdmin (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'id': 'halt-admin-box'})

        submit = CTK.Submitter('/halt')
        submit += CTK.Hidden ('what', 'ever')

        dialog = CTK.Dialog ({'title': _('Shutdown Cherokee-admin'), 'width': 560})
        dialog.AddButton (_('Cancel'), "close")
        dialog.AddButton (_('Shut down'), dialog.JS_to_trigger('submit'))
        dialog += CTK.RawHTML ("<h2>%s</h2>" %(_('You are about to shut down this instance of Cherokee-admin')))
        dialog += CTK.RawHTML ("<p>%s</p>" %(_('Are you sure you want to proceed?')))
        dialog += submit
        dialog.bind ('submit', dialog.JS_to_close())
        dialog.bind ('submit', "$('body').html('<h1>%s</h1>');"%(_('Cherokee-admin has been shut down')))

        link = CTK.Link (None, CTK.RawHTML (_('Shut down Cherokee-Admin')))
        link.bind ('click', dialog.JS_to_show())

        self += link
        self += dialog


class Render:
    def __call__ (self):
        Cherokee.pid.refresh()

        # Top
        top = CTK.Box({'id': 'top-box'})
        top += CTK.RawHTML ("<h1>%s</h1>"% _('Welcome to Cherokee Admin'))
        top += LanguageSelector()

        # Content: Left
        mainarea = CTK.Box({'id': 'main-area'})
        mainarea += ServerInfo()
        mainarea += CPUInfo()
        mainarea += MemoryInfo()
        mainarea += CommunityBar()

        # Content: Right
        sidebar = CTK.Box({'id': 'sidebar'})
        sidebar += SupportBox()
        sidebar += HaltAdmin()

        # Content
        cont = CTK.Box({'id': 'home-container'})
        cont += mainarea
        cont += sidebar

        # Page
        page = Page.Base(_('Welcome to Cherokee Admin'), body_id='index', helps=HELPS)
        page += top
        page += cont
        page += CTK.RawHTML (js=JS_SCROLL)

        return page.Render()


CTK.publish (r'^/$',       Render)
CTK.publish (r'^/launch$', Launch)
CTK.publish (r'^/stop$',   Stop)
CTK.publish (r'^/halt$',   Halt)
CTK.publish (r'^/lang/apply',             Lang_Apply,           method="POST")
