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

import CTK

URL_APPLY = '/plugin/rehost/apply'

NOTE_REHOST   = N_("Regular Expression against which the hosts be Host name will be compared.")
WARNING_EMPTY = N_("At least one Regular Expression string must be defined.")


class Content (CTK.Container):
    def __init__ (self, refreshable, key, url_apply, **kwargs):
        CTK.Container.__init__ (self, **kwargs)
        entries = CTK.cfg.keys (key)

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
            table += [CTK.RawHTML(_('Regular Expressions'))]

            for i in entries:
                e1 = CTK.TextCfg ("%s!%s"%(key,i))
                rm = None
                if len(entries) >= 2:
                    rm = CTK.ImageStock('del')
                    rm.bind('click', CTK.JS.Ajax (url_apply,
                                                  data     = {"%s!%s"%(key,i): ''},
                                                  complete = refreshable.JS_to_refresh()))
                table += [e1, rm]

        # Add New
        table = CTK.PropsTable()
        next  = CTK.cfg.get_next_entry_prefix (key)
        table.Add (_('New Regular Expression'), CTK.TextCfg(next, False, {'class':'noauto'}), _(NOTE_REHOST))

        submit = CTK.Submitter(url_apply)
        dialog = CTK.Dialog2Buttons ({'title': _('Add New Entry')}, _('Add'), submit.JS_to_submit())

        submit += table
        submit.bind ('submit_success', refreshable.JS_to_refresh())
        submit.bind ('submit_success', dialog.JS_to_close())

        dialog += submit
        self += dialog

        add_new = CTK.Button(_('Add new entry…'))
        add_new.bind ('click', dialog.JS_to_show())
        self += add_new


class Plugin_rehost (CTK.Plugin):
    def __init__ (self, key, vsrv_num):
        CTK.Plugin.__init__ (self, key)

        pre       = '%s!regex' %(key)
        url_apply = '%s/%s' %(URL_APPLY, vsrv_num)

        self += CTK.RawHTML ("<h2>%s</h2>" % (_('New Regular Expression')))

        # Content
        refresh = CTK.Refreshable ({'id': 'plugin_rehost'})
        refresh.register (lambda: Content(refresh, pre, url_apply).Render())
        self += refresh

        # Validation, and Public URLs
        CTK.publish ('^%s/[\d]+'%(URL_APPLY), CTK.cfg_apply_post, method="POST")
