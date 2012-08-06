# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#      Taher Shihadeh <taher@octality.com>
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
import Cherokee
import xmlrpclib
import XMLServerDigest
import PageError
import SystemStats
import SystemStatsWidgets
import About

import os
import time
import urllib
import urllib2
import re

import OWS_Login
import OWS_Backup
import OWS_Market_Info
import OWS_Cherokee_Info

from util import *
from consts import *
from configured import *

# URLs
LINK_OCTALITY   = 'http://www.octality.com/'
LINK_SUPPORT    = 'http://www.octality.com/engineering.html'
OWS_PROUD       = 'http://www.octality.com/api/v%s/open/proud/' %(OWS_API_VERSION)
PROUD_USERS_WEB = "http://www.cherokee-project.com/cherokee-domain-list.html"

# Links
LINK_BUGTRACKER = 'http://bugs.cherokee-project.com/'
LINK_TWITTER    = 'http://twitter.com/webserver'
LINK_FACEBOOK   = 'http://www.facebook.com/cherokee.project'
LINK_DOWNLOAD   = 'http://www.cherokee-project.com/download/'
LINK_LISTS      = 'http://lists.octality.com/'
LINK_LIST       = 'http://lists.octality.com/listinfo/cherokee'
LINK_IRC        = 'irc://irc.freenode.net/cherokee'
LINK_HELP       = '/help/basics.html'
LINK_CHEROKEE   = 'http://www.cherokee-project.com/'
LINK_LINKEDIN   = 'http://www.linkedin.com/groups?gid=1819726'

# Subscription
SUBSCRIBE_URL       = 'http://lists.octality.com/subscribe/cherokee-dev'
SUBSCRIBE_CHECK     = 'Your subscription request has been received'
SUBSCRIBE_APPLY     = '/index/subscribe/apply'

NOTE_EMAIL          = N_("You will be sent an email requesting confirmation")
NOTE_NAME           = N_("Optionally provide your name")

MAILING_LIST_INFO   = N_("""\
There are a number of Community <a href="%(link)s" target="_blank">Mailing Lists</a>
available for you to subscribe. You can subscribe the General Discussion
mailing list from this interface. There is where most of the discussions
take place.""")%({'link':LINK_LISTS})

# Server is..
RUNNING_NOTICE      = N_('Server is Running')
STOPPED_NOTICE      = N_('Server is not Running')

PROUD_USERS_NOTICE  = N_("""\
We would love to know that you are using Cherokee. Submit your domain
name and it will be listed on the Cherokee Project web site.
""")

# Dialogs
PROUD_DIALOG_OK     = N_("The information has been successfully sent. Thank you!")
PROUS_DIALOG_ERROR1 = N_("Unfortunatelly something went wrong, and the information could not be submitted:")
PROUS_DIALOG_ERROR2 = N_("Please, try again. Do not hesitate to report the problem if it persists.")

#
REMOTE_SERVS_APPLY  = '/remote-servs/apply'
REMOTE_SERVS_ENABLE = N_('Enable Remote Services')


# Help entries
HELPS = [('config_status', N_("Status"))]

# JS
JS_SUBSCRIBE = """
$('#subscribe-a').click (function(){ %s });
"""
JS_PROUD = """
$('#proud-a').click (function(){ %s });
"""

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


def RemoteServices_Apply():
    enabled = CTK.post.get_val('admin!ows!enabled')
    CTK.cfg['admin!ows!enabled'] = enabled
    return CTK.cfg_reply_ajax_ok()


class RemoteServices (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'id': 'remote-services-section', 'class': 'infosection'})

        submit = CTK.Submitter (REMOTE_SERVS_APPLY)
        submit += CTK.CheckCfgText ("admin!ows!enabled", True, _(REMOTE_SERVS_ENABLE))
        submit.bind ('submit_success', CTK.JS.GotoURL('/'))

        infotable = CTK.Table({'class': 'info-table'})
        infotable.set_header (column=True, num=1)

        if int (CTK.cfg.get_val("admin!ows!enabled", OWS_ENABLE)):
            if OWS_Login.is_logged():
                infotable += [submit, OWS_Login.LoggedAs_Text()]
            else:
                dialog = OWS_Login.LoginDialog()
                dialog.bind ('submit_success', CTK.JS.GotoURL('/'))

                link = CTK.Link ("#", CTK.RawHTML('<span>%s</span>' %(_('Sign in'))))
                link.bind ('click', dialog.JS_to_show())

                cont = CTK.Container()
                cont += dialog
                cont += link

                infotable += [submit, cont]
        else:
            infotable += [submit]

        table = CTK.Table()
        table.set_header (column=True, num=1)
        table += [CTK.RawHTML (_('Remote Services')), infotable]
        self += table

class BackupService (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'id': 'remote-backup-section', 'class': 'infosection'})

        cont = CTK.Container()
        cont += OWS_Backup.Restore_Config_Button()
        cont += OWS_Backup.Save_Config_Button()

        table = CTK.Table()
        table.set_header (column=True, num=1)
        table += [CTK.RawHTML (_('Backup Service')), cont]
        self += table


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

    OWS_Market_Info.invalidate_caches()
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


def ProudUsers_Apply():
    # Collect domains
    domains = []

    for v in CTK.cfg.keys('vserver'):
        domains.append (CTK.cfg.get_val('vserver!%s!nick'%(v)))

        for d in CTK.cfg.keys('vserver!%s!match!domain'%(v)):
            domains.append (CTK.cfg.get_val('vserver!%s!match!domain!%s'%(v, d)))

        for d in CTK.cfg.keys('vserver!%s!match!regex'%(v)):
            domains.append (CTK.cfg.get_val('vserver!%s!match!regex!%s'%(v, d)))

    # Send the list
    try:
        xmlrpc = XMLServerDigest.XmlRpcServer (OWS_PROUD)
        xmlrpc.add_domains_to_review(domains)

    except xmlrpclib.ProtocolError, err:
        details  = "Error code:    %d\n" % err.errcode
        details += "Error message: %s\n" % err.errmsg
        details += "Headers: %s\n"       % err.headers

        return '<p>%s</p>'            %(_(PROUS_DIALOG_ERROR1))    + \
               '<p><pre>%s</pre></p>' %(CTK.escape_html(details))  + \
               '<p>%s</p>'            %(_(PROUS_DIALOG_ERROR2))

    except Exception, e:
        return '<p>%s</p>'              %(_(PROUS_DIALOG_ERROR1))  + \
               '<p><pre>%s\n</pre></p>' %(CTK.escape_html(str(e))) + \
               '<p>%s</p>'              %(_(PROUS_DIALOG_ERROR2))

    return "<p>%s</p>" %(_(PROUD_DIALOG_OK))


class ProudUsers (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'id': 'proud-users', 'class': 'sidebar-box'})

        # Dialog
        dialog = CTK.DialogProxyLazy ('/proud/apply', {'title': _('Proud Cherokee User List Submission'), 'width': 480})
        dialog.AddButton (_('Close'), "close")

        self += CTK.RawHTML('<h2>%s</h2>' %(_('Proud Cherokee Users')))
        self += CTK.Box ({'id': 'proud-notice'}, CTK.RawHTML (_(PROUD_USERS_NOTICE)))
        self += CTK.Box ({'id': 'proud-link'},   CTK.RawHTML ('<a target="_blank" href="%s">%s</a> | <a id="proud-a">%s</a>' %(_(PROUD_USERS_WEB), _('View list…'), _('Send your domains'))))
        self += CTK.RawHTML (js=JS_PROUD %(dialog.JS_to_show()))
        self += dialog


def Subscribe_Apply ():
    values = {}
    for k in CTK.post:
        values[k] = CTK.post[k]

    data = urllib.urlencode(values)
    req  = urllib2.Request(SUBSCRIBE_URL, data)
    response = urllib2.urlopen(req)
    results_page = response.read()
    if SUBSCRIBE_CHECK in results_page:
        return {'ret':'ok'}

    return {'ret':'error'}


class MailingListDialog (CTK.Dialog):
    def __init__ (self):
        CTK.Dialog.__init__ (self, {'title': _('Mailing List Subscription'), 'width': 560})
        self.AddButton (_('Cancel'), "close")
        self.AddButton (_('Subscribe'), self.JS_to_trigger('submit'))

        table = CTK.PropsTable()
        table.Add (_('Your email address'), CTK.TextField({'name': 'email', 'class': 'noauto'}), _(NOTE_EMAIL))
        table.Add (_('Your name'),          CTK.TextField({'name': 'fullname', 'class': 'noauto', 'optional':True}), _(NOTE_NAME))

        submit = CTK.Submitter (SUBSCRIBE_APPLY)
        submit.bind ('submit_success', self.JS_to_close())
        submit += table

        self += CTK.RawHTML ("<p>%s</p>" %(_(MAILING_LIST_INFO)))
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
        link = CTK.Link ('#', CTK.RawHTML (_('Subscribe to mailing lists')))
        dialog = MailingListDialog()

        link.bind ('click', dialog.JS_to_show())
        self += dialog
        qlist += link

        # Bug report
        link = CTK.LinkWindow (LINK_BUGTRACKER, CTK.RawHTML (_('Report a bug')))
        qlist += link

        # Commercial Support
        link = CTK.LinkWindow (LINK_SUPPORT, CTK.RawHTML (_('Purchase commercial support')))
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

        slist += CTK.LinkWindow (LINK_CHEROKEE, CTK.Image({'src': '/static/images/other/web.png',      'title': _('Visit the Cherokee Project Website')}))
        slist += CTK.LinkWindow (LINK_TWITTER,  CTK.Image({'src': '/static/images/other/twitter.png',  'title': _('Follow us on Twitter')}))
        slist += CTK.LinkWindow (LINK_FACEBOOK, CTK.Image({'src': '/static/images/other/facebook.png', 'title': _('Join us on Facebook')}))
        slist += CTK.LinkWindow (LINK_LINKEDIN, CTK.Image({'src': '/static/images/other/linkedin.png', 'title': _('Become a member of Cherokee group on LinkedIn')}))
        slist += CTK.LinkWindow (LINK_IRC,      CTK.Image({'src': '/static/images/other/irc.png',      'title': _('Chat with us at irc.freenode.net')}))

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
        mainarea += OWS_Market_Info.Index_Block1()
        mainarea += ServerInfo()
        if int(OWS_ENABLE):
            mainarea += RemoteServices()

        if OWS_Login.is_logged() and \
           int (CTK.cfg.get_val("admin!ows!enabled", OWS_ENABLE)):
            mainarea += BackupService()

        mainarea += CPUInfo()
        mainarea += MemoryInfo()
        mainarea += CommunityBar()

        # Content: Right
        sidebar = CTK.Box({'id': 'sidebar'})
        sidebar += SupportBox()

        if int (CTK.cfg.get_val("admin!ows!enabled", OWS_ENABLE)):
            sidebar += OWS_Cherokee_Info.Latest_Release()

        sidebar += ProudUsers()
        sidebar += OWS_Market_Info.Index_Block2()
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
CTK.publish (r'^/proud/apply',            ProudUsers_Apply,     method="POST")
CTK.publish (r'^%s'%(SUBSCRIBE_APPLY),    Subscribe_Apply,      method="POST")
CTK.publish (r'^%s'%(REMOTE_SERVS_APPLY), RemoteServices_Apply, method="POST")
