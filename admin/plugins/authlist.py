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
import Auth
from CTK.i18n import _, N_
import validations

URL_APPLY = '/plugin/authlist/apply'
HELPS     = [('modules_validators_authlist', _("Fixed list"))]

NOTE_EMPTY = N_("At least one user/password pair should be configured.")


def commit():
    new_user = CTK.post.pop('tmp!new_user')
    new_pass = CTK.post.pop('tmp!new_pass')
    key      = CTK.post.pop('key')

    # New
    if new_user:
        next = CTK.cfg.get_next_entry_prefix (key)
        CTK.cfg[f'{next}!user']     = new_user
        CTK.cfg[f'{next}!password'] = new_pass
        return CTK.cfg_reply_ajax_ok()

    # Modification
    return CTK.cfg_apply_post()


class Plugin_authlist (Auth.PluginAuth):
    class Content (CTK.Container):
        def __init__ (self, refresh, key):
            CTK.Container.__init__ (self)

            keys = CTK.cfg.keys (f'{key}!list')
            keys.sort(key=int)

            if keys:
                self += CTK.RawHTML (f"<h3>{_('Authentication list')}</h3>")
                table = CTK.Table({'id': 'authlist-table'})
                table.set_header (1)
                table += [CTK.RawHTML(x) for x in (_('User'), _('Password'),'')]
                for c in keys:
                    pre    = f'{key}!list!{c}'
                    user   = CTK.RawHTML (CTK.cfg.get_val(f'{pre}!user'))
                    passwd = CTK.RawHTML (CTK.cfg.get_val(f'{pre}!password'))
                    remove = None
                    if len(keys) >= 2:
                        remove = CTK.ImageStock('del')
                        remove.bind('click', CTK.JS.Ajax (URL_APPLY,
                                                          data     = {pre: ''},
                                                          complete = refresh.JS_to_refresh()))
                    table += [user, passwd, remove]
                self += CTK.Indenter (table)
            else:
                self += CTK.Indenter (CTK.Notice ('information', CTK.RawHTML(_(NOTE_EMPTY))))

    def __init__ (self, key, **kwargs):
        Auth.PluginAuth.__init__ (self, key, **kwargs)
        self.AddCommon (supported_methods=('basic','digest'))

        # List
        refresh = CTK.Refreshable ({'id': 'authlist'})
        refresh.register (lambda: self.Content(refresh, key).Render())

        self += CTK.RawHTML (f"<h2>{_('Fixed Authentication List')}</h2>")
        self += refresh

        # New
        new_user = CTK.TextField({'name': 'tmp!new_user', 'class': 'noauto'})
        new_pass = CTK.TextField({'name': 'tmp!new_pass', 'class': 'noauto'})
        new_add  = CTK.SubmitterButton (_('Add'))

        table = CTK.Table()
        table.set_header(1)
        table += [CTK.RawHTML(x) for x in (_('User'), _('Password'))]
        table += [new_user, new_pass, new_add]

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('key', f'{key}!list')
        submit += table
        submit.bind ('submit_success', refresh.JS_to_refresh() + new_user.JS_to_clean() + new_pass.JS_to_clean())

        self += CTK.RawHTML (f"<h3>{_('Add New Pair')}</h3>")
        self += CTK.Indenter (submit)

        # Publish
        VALS = [(f"{self.key}!passwdfile", validations.is_local_file_exists)]
        CTK.publish (f'^{URL_APPLY}', commit, validation=VALS, method="POST")
