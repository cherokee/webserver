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
import XMLServerDigest

from configured import *


LOGIN_APPLY      = '/login/apply'
SIGNOUT_APPLY    = '/signout/apply'

OWS_WEB_REGISTER = 'http://cherokee-market.com/signup'
OWS_WEB_LOGIN    = 'https://www.octality.com/api/v%s/login' %(OWS_API_VERSION)
OWS_WEB_USER     = 'http://cherokee-market.com/user/%s'

NOTE_REGISTER    = N_('<span class="register-link">Not registered yet? <a target = "_blank" href="%s">Join today</a>.</span>')

login_user       = None
login_password   = None
login_session    = None

def log_in (user, password):
    global login_user, login_password, login_session

    if is_logged():
        return True

    # NOTE: This request uses the provided login/password pair to
    # access the web service. It requires Digest authentication. It
    # will fail if the pair is not valid.
    #
    try:
        xmlrpc = XMLServerDigest.XmlRpcServer (OWS_WEB_LOGIN, user, password)
        session = xmlrpc.is_logged()
        if session:
            login_user     = user
            login_password = password
            login_session  = session
            CTK.cfg["admin!ows!login!user"]     = user
            CTK.cfg["admin!ows!login!password"] = password
            return True
    except:
        CTK.util.print_exception()
        return None

    return False


def is_logged():
    global login_user, login_password

    if login_user and login_password:
        return True

    return False


def Login_Apply():
    # Validation
    if not CTK.post['email']:
        return {'ret':'error', 'errors': {'email': _("Can not be empty")}}
    if not CTK.post['password']:
        return {'ret':'error', 'errors': {'password': _("Can not be empty")}}

    # Authenticate
    prev_changes = CTK.cfg.has_changed()

    logged = log_in (CTK.post['email'], CTK.post['password'])
    if logged == True:
        # Auto-save it there were no prev changes
        if not prev_changes:
            CTK.cfg.save()

        return {'ret':'ok'}

    return {'ret':'error', 'errors': {'password': _("Authentication failed")}}

def SignOut_Apply():
    global login_user, login_password
    login_user     = None
    login_password = None
    return CTK.HTTP_Redir("/")


class LoginDialog (CTK.Dialog):
    def __init__ (self):
        CTK.Dialog.__init__ (self, {'title': _("Sign in to Cherokee Market")}, {'id': 'login-dialog'})

        submit  = CTK.Submitter (LOGIN_APPLY)
        submit += CTK.RawHTML (_('Email address'))
        submit += CTK.TextField ({'name': 'email', 'class': 'noauto'})
        submit += CTK.RawHTML (_('Password'))
        submit += CTK.TextFieldPassword ({'name': 'password', 'class': 'noauto'})
        submit += CTK.SubmitterButton (_("Sign in"))

        bottompanel = CTK.Box ({'id': 'login-bottom'})
        bottompanel += CTK.RawHTML (_(NOTE_REGISTER) %(OWS_WEB_REGISTER))

        self += submit
        self += bottompanel

class LoggedAs_Text (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class': 'login-box-logged-in'})

        self += CTK.RawHTML('%s ' %(_("Logged as")))
        self += CTK.LinkWindow (OWS_WEB_USER %(login_user), CTK.RawHTML(login_user))
        self += CTK.RawHTML(" (")
        self += CTK.Link (SIGNOUT_APPLY, CTK.RawHTML(_("Sign out")))
        self += CTK.RawHTML(")")


CTK.publish (r'^%s' %(LOGIN_APPLY), Login_Apply, method="POST")
CTK.publish (r'^%s$'%(SIGNOUT_APPLY), SignOut_Apply)
