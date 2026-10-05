# -*- coding: utf-8 -*-
#
# Cheroke-admin
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

import re
import CTK
import validations
from CTK.i18n import _, N_

URL_APPLY = '/plugin/target_ip/apply'

NOTE_ADDRESS  = N_("IP or Subnet of the NIC that accepted the request. Example: ::1, or 10.0.0.0/8")
WARNING_EMPTY = N_("At least one IP or Subnet entry must be defined.")

def apply_del():
    for k in CTK.post:
        del (CTK.cfg[k])

    return CTK.cfg_reply_ajax_ok()


class AddressesTable (CTK.Container):
    def __init__ (self, refreshable, key, url_apply, **kwargs):
        CTK.Container.__init__ (self, **kwargs)
        entries = CTK.cfg.keys(key)

        # Warning message
        if not entries:
            notice = CTK.Notice('warning')
            notice += CTK.RawHTML (_(WARNING_EMPTY))
            self += notice

        # List
        else:
            table  = CTK.Table()
            submit = CTK.Submitter(url_apply)

            submit += table
            self += CTK.Indenter(submit)

            table.set_header(1)
            table += [CTK.RawHTML(_('IP or Subnet'))]

            for i in entries:
                e1 = CTK.TextCfg (f"{key}!{i}")
                rm = None
                if len(entries) >= 2:
                    rm = CTK.ImageStock('del')
                    rm.bind('click', CTK.JS.Ajax (f'{url_apply}/del',
                                                  data     = {f"{key}!{i}": ''},
                                                  complete = refreshable.JS_to_refresh()))
                table += [e1, rm]

        # Add New
        table = CTK.PropsTable()
        next  = CTK.cfg.get_next_entry_prefix (key)
        table.Add (_('New IP/Subnet'), CTK.TextCfg(next, False, {'class':'noauto'}), _(NOTE_ADDRESS))

        submit = CTK.Submitter(url_apply)
        dialog = CTK.Dialog2Buttons ({'title': _('Add new entry')}, _('Add'), submit.JS_to_submit())

        submit += table
        submit.bind ('submit_success', refreshable.JS_to_refresh())
        submit.bind ('submit_success', dialog.JS_to_close())

        dialog += submit
        self += dialog

        add_new = CTK.Button(_('Add new IP/Subnet…'))
        add_new.bind ('click', dialog.JS_to_show())
        self += add_new


class Plugin_target_ip (CTK.Plugin):
    def __init__ (self, key, vsrv_num):
        CTK.Plugin.__init__ (self, key)

        pre       = f'{key}!to'
        url_apply = f'{URL_APPLY}/{vsrv_num}'

        self += CTK.RawHTML (f"<h2>{_('Accepted Server IP Addresses and Subnets')}</h2>")

        # Content
        refresh = CTK.Refreshable ({'id': 'plugin_target_ip'})
        refresh.register (lambda: AddressesTable(refresh, pre, url_apply).Render())
        self += refresh

        # Validation, and Public URLs
        VALS = [(f'{pre}!.+', validations.is_ip_or_netmask)]
        CTK.publish (f'^{URL_APPLY}/[\\d]+', CTK.cfg_apply_post, method="POST", validation=VALS)
        CTK.publish (f'^{URL_APPLY}/[\\d]+/del', apply_del,          method="POST")
