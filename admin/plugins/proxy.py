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
import Handler
from CTK.i18n import _, N_
import Cherokee
import Balancer
import validations

from util import *
from consts import *

URL_APPLY = '/plugin/proxy/apply'
HELPS     = [('modules_handlers_proxy', N_("Reverse Proxy"))]

NOTE_REUSE_MAX       = N_("Maximum number of connections per server that the proxy can try to keep opened.")
NOTE_ALLOW_KEEPALIVE = N_("Allow the server to use Keep-alive connections with the back-end servers.")
NOTE_PRESERVE_HOST   = N_("Preserve the original \"Host:\" header sent by the client. (Default: No)")
NOTE_PRESERVE_SERVER = N_("Preserve the \"Server:\" header sent by the back-end server. (Default: No)")
NOTE_ERROR_HANDLER   = N_("Use the VServer error handler, whenever an error response is received from a back-end server. (Default: No)")

VALS = [
    ('.+?!reuse_max', validations.is_number_gt_0),
]


def commit():
    # New Rewrite
    for e in ('in_rewrite_request', 'out_rewrite_request'):
        key   = CTK.post.pop (f'tmp!new!{e}!key')
        regex = CTK.post.pop (f'tmp!new!{e}!regex')
        subst = CTK.post.pop (f'tmp!new!{e}!substring')

        if regex:
            next = CTK.cfg.get_next_entry_prefix (f'{key}!{e}')
            CTK.cfg[f'{next}!regex']     = regex
            CTK.cfg[f'{next}!substring'] = subst
            return CTK.cfg_reply_ajax_ok()

    # New Header
    for e in ('in_header_add', 'out_header_add'):
        key    = CTK.post.pop (f'tmp!new!{e}!key')
        header = CTK.post.pop (f'tmp!new!{e}!header')
        value  = CTK.post.pop (f'tmp!new!{e}!value')

        if header and value:
            CTK.cfg[f'{key}!{e}!{header}'] = value
            return CTK.cfg_reply_ajax_ok()

    # New Hide
    for e in ('in_header_hide', 'out_header_hide'):
        key  = CTK.post.pop (f'tmp!new!{e}!key')
        hide = CTK.post.pop (f'tmp!new!{e}!hide')

        if hide:
            next = CTK.cfg.get_next_entry_prefix (f'{key}!{e}')
            CTK.cfg[next] = hide
            return CTK.cfg_reply_ajax_ok()

    # Modification
    return CTK.cfg_apply_post()


class URL_Rewrite (CTK.Container):
    class Content (CTK.Container):
        def __init__ (self, refresh, key):
            CTK.Container.__init__ (self)

            keys = CTK.cfg.keys(key)
            if keys:
                table = CTK.Table()
                table.set_header(1)
                table += [CTK.RawHTML(x) for x in (_('Regular Expression'), _('Substitution'))]

                for k in CTK.cfg.keys(key):
                    regex = CTK.TextCfg (f'{key}!{k}!regex', False)
                    subst = CTK.TextCfg (f'{key}!{k}!substring', False)
                    remove = CTK.ImageStock('del')
                    remove.bind('click', CTK.JS.Ajax (URL_APPLY, data={f'{key}!{k}': ''},
                                                          complete = refresh.JS_to_refresh()))
                    table += [regex, subst, remove]

                submit = CTK.Submitter (URL_APPLY)
                submit += table

                self += CTK.Indenter (submit)

    def __init__ (self, key, key_entry):
        CTK.Container.__init__ (self)

        # List
        refresh = CTK.Refreshable ({'id': f'proxy_{key_entry}'})
        refresh.register (lambda: self.Content(refresh, f'{key}!{key_entry}').Render())
        self += refresh

        # New
        new_regex  = CTK.TextCfg(f'tmp!new!{key_entry}!regex', False, {'class': 'noauto'})
        new_subst  = CTK.TextCfg(f'tmp!new!{key_entry}!substring', False, {'class': 'noauto'})
        add_button = CTK.SubmitterButton(_('Add'))

        table = CTK.Table()
        table.set_header(1)
        table += [CTK.RawHTML(x) for x in (_('Add RegEx'), _('Substitution'))]
        table += [new_regex, new_subst, add_button]

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Indenter (table)
        submit += CTK.Hidden (f'tmp!new!{key_entry}!key', key)
        submit.bind ('submit_success', refresh.JS_to_refresh())

        self += submit


class Header_List (CTK.Container):
    class Content (CTK.Container):
        def __init__ (self, refresh, key):
            CTK.Container.__init__ (self)

            keys = CTK.cfg.keys(key)
            if keys:
                table = CTK.Table()
                table.set_header(1)
                table += [CTK.RawHTML(x) for x in (_('Regular Expression'), _('Substitution'))]

                for k in CTK.cfg.keys(key):
                    value = CTK.TextCfg (f'{key}!{k}', False)
                    remove = CTK.ImageStock('del')
                    remove.bind('click', CTK.JS.Ajax (URL_APPLY, data={f'{key}!{k}': ''},
                                                      complete = refresh.JS_to_refresh()))
                    table += [CTK.RawHTML(k), value, remove]

                submit = CTK.Submitter (URL_APPLY)
                submit += table

                self += CTK.Indenter (submit)

    def __init__ (self, key, key_entry):
        CTK.Container.__init__ (self)

        # List
        refresh = CTK.Refreshable ({'id': f'proxy_{key_entry}'})
        refresh.register (lambda: self.Content(refresh, f'{key}!{key_entry}').Render())
        self += refresh

        # New
        new_regex  = CTK.TextCfg(f'tmp!new!{key_entry}!header', False, {'class': 'noauto'})
        new_subst  = CTK.TextCfg(f'tmp!new!{key_entry}!value', False, {'class': 'noauto'})
        add_button = CTK.SubmitterButton(_('Add'))

        table = CTK.Table()
        table.set_header(1)
        table += [CTK.RawHTML(x) for x in (_('Add Header Entry'), _('Value'))]
        table += [new_regex, new_subst, add_button]

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Indenter (table)
        submit += CTK.Hidden (f'tmp!new!{key_entry}!key', key)
        submit.bind ('submit_success', refresh.JS_to_refresh())

        self += submit


class Header_Hide (CTK.Container):
    class Content (CTK.Container):
        def __init__ (self, refresh, key):
            CTK.Container.__init__ (self)

            keys = CTK.cfg.keys(key)
            if keys:
                table = CTK.Table()
                table.set_header(1)
                table += [CTK.RawHTML(_('Header'))]

                for k in CTK.cfg.keys(key):
                    remove = CTK.ImageStock('del')
                    remove.bind('click', CTK.JS.Ajax (URL_APPLY, data={f'{key}!{k}': ''},
                                                          complete = refresh.JS_to_refresh()))
                    table += [CTK.RawHTML(CTK.cfg.get_val(f'{key}!{k}')), remove]

                submit = CTK.Submitter (URL_APPLY)
                submit += table

                self += CTK.Indenter (submit)

    def __init__ (self, key, key_entry):
        CTK.Container.__init__ (self)

        # List
        refresh = CTK.Refreshable ({'id': f'proxy_{key_entry}'})
        refresh.register (lambda: self.Content(refresh, f'{key}!{key_entry}').Render())
        self += refresh

        # New
        new_hide  = CTK.TextCfg(f'tmp!new!{key_entry}!hide', False, {'class': 'noauto'})
        add_button = CTK.SubmitterButton(_('Add'))

        table = CTK.Table()
        table.set_header(1)
        table += [CTK.RawHTML (_('Hide Header'))]
        table += [new_hide, add_button]

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Indenter (table)
        submit += CTK.Hidden (f'tmp!new!{key_entry}!key', key)
        submit.bind ('submit_success', refresh.JS_to_refresh())

        self += submit


class Plugin_proxy (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        kwargs['show_document_root'] = False
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        # Properties
        table = CTK.PropsTable()
        table.Add (_('Reuse connections'),         CTK.TextCfg (f'{key}!reuse_max', True), _(NOTE_REUSE_MAX))
        table.Add (_('Allow Keepalive'),           CTK.CheckCfgText(f'{key}!in_allow_keepalive',  True,  _('Allow')),    _(NOTE_ALLOW_KEEPALIVE))
        table.Add (_('Preserve Host Header'),      CTK.CheckCfgText(f'{key}!in_preserve_host',    False, _('Preserve')), _(NOTE_PRESERVE_HOST))
        table.Add (_('Preserve Server Header'),    CTK.CheckCfgText(f'{key}!out_preserve_server', False, _('Preserve')), _(NOTE_PRESERVE_SERVER))
        table.Add (_('Use VServer error handler'), CTK.CheckCfgText(f'{key}!vserver_errors',      False, _('Use it')),   _(NOTE_ERROR_HANDLER))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML (f"<h2>{_('Reverse Proxy Settings')}</h2>")
        self += CTK.Indenter (submit)

        # Request
        self += CTK.RawHTML (f"<h2>{_('Request')}</h2>")

        self += CTK.Indenter (CTK.RawHTML (f"<h3>{_('URL Rewriting')}</h3>"))
        self += URL_Rewrite (key, 'in_rewrite_request')

        self += CTK.Indenter (CTK.RawHTML (f"<h3>{_('Header Addition')}</h3>"))
        self += Header_List (key, 'in_header_add')

        self += CTK.Indenter (CTK.RawHTML (f"<h3>{_('Hide Headers')}</h3>"))
        self += Header_Hide (key, 'in_header_hide')

        # Reply
        self += CTK.RawHTML (f"<h2>{_('Reply')}</h2>")

        self += CTK.Indenter (CTK.RawHTML (f"<h3>{_('URL Rewriting')}</h3>"))
        self += URL_Rewrite (key, 'out_rewrite_request')

        self += CTK.Indenter (CTK.RawHTML (f"<h3>{_('Header Addition')}</h3>"))
        self += Header_List (key, 'out_header_add')

        self += CTK.Indenter (CTK.RawHTML (f"<h3>{_('Hide Headers')}</h3>"))
        self += Header_Hide (key, 'out_header_hide')

        # Balancer
        modul = CTK.PluginSelector(f'{key}!balancer', trans_options(Cherokee.support.filter_available (BALANCERS)))
        table = CTK.PropsTable()
        table.Add (_("Balancer"), modul.selector_widget, _(Balancer.NOTE_BALANCER))

        self += CTK.RawHTML (f"<h2>{_('Back-end Servers')}</h2>")
        self += CTK.Indenter (table)
        self += modul

CTK.publish (f'^{URL_APPLY}', commit, method="POST", validation=VALS)
