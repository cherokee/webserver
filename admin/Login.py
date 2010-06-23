# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Taher Shihadeh
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

LOGIN_APPLY      = '/login/apply/'
OWS_REGISTER_URL = 'http://mall.octality.com/signup'
OWS_LOGIN        = 'https://www.octality.com/api/login/'

NOTE_EMAIL       = N_('Your email address is used as user account')
NOTE_PASSWORD    = N_('Password of your account')
NOTE_REGISTER    = N_('Not registered yet? <a target = "_blank" href="%s">Join today</a>.')

login_user       = None
login_password   = None

def log_in (user, password):
    global login_user, login_password
    if is_logged():
        return True

    try:
        xmlrpc = XMLServerDigest.XmlRpcServer (OWS_LOGIN, user, password)
        if xmlrpc.is_logged():
            login_user     = user
            login_password = password
            return True
    except:
        return None

    return False


def is_logged ():
    global login_user, login_password
    if login_user and login_password:
        return True

    return False


def Login_Apply ():
    logged = log_in (CTK.post['email'], CTK.post['password'])

    if logged == True:
        return {'ret':'ok'}
    return {'ret':'fail'}


class Login_Form (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        table = CTK.PropsTable()
        table.Add (_('Email address'), CTK.TextField({'name': 'email', 'class': 'noauto'}), _(NOTE_EMAIL))
        table.Add (_('Password'),      CTK.TextFieldPassword({'name': 'password', 'class': 'noauto'}), _(NOTE_PASSWORD))

        submit = CTK.Submitter(LOGIN_APPLY)
        submit += table
        self += submit
        self += CTK.RawHTML(_(NOTE_REGISTER) %(OWS_REGISTER_URL))


class Login_Button (CTK.Box):
    def __init__ (self, label=_('Login')):
        CTK.Box.__init__ (self, {'class': 'login-button'})

        dialog = CTK.Dialog ({'title': _('Octality Account Login'), 'width': 500})
        dialog.AddButton (_('Sign in'), dialog.JS_to_trigger('submit'))
        dialog.AddButton (_('Close'), "close")
        dialog += Login_Form()

        button = CTK.Button(label)
        button.bind ('click', dialog.JS_to_show())
        dialog.bind ('submit_success', dialog.JS_to_close())
        dialog.bind ('submit_success', self.JS_to_trigger('submit_success'));

        self += button
        self += dialog

CTK.publish (r'^%s$'%(LOGIN_APPLY), Login_Apply, method="POST")
