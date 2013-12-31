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
        key   = CTK.post.pop ('tmp!new!%s!key'%(e))
        regex = CTK.post.pop ('tmp!new!%s!regex'%(e))
        subst = CTK.post.pop ('tmp!new!%s!substring'%(e))

        if regex:
            next = CTK.cfg.get_next_entry_prefix ('%s!%s'%(key, e))
            CTK.cfg['%s!regex'%(next)]     = regex
            CTK.cfg['%s!substring'%(next)] = subst
            return CTK.cfg_reply_ajax_ok()

    # New Header
    for e in ('in_header_add', 'out_header_add'):
        key    = CTK.post.pop ('tmp!new!%s!key'%(e))
        header = CTK.post.pop ('tmp!new!%s!header'%(e))
        value  = CTK.post.pop ('tmp!new!%s!value'%(e))

        if header and value:
            CTK.cfg['%s!%s!%s'%(key, e, header)] = value
            return CTK.cfg_reply_ajax_ok()

    # New Hide
    for e in ('in_header_hide', 'out_header_hide'):
        key  = CTK.post.pop ('tmp!new!%s!key'%(e))
        hide = CTK.post.pop ('tmp!new!%s!hide'%(e))

        if hide:
            next = CTK.cfg.get_next_entry_prefix ('%s!%s'%(key, e))
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
                    regex = CTK.TextCfg ('%s!%s!regex'%(key,k), False)
                    subst = CTK.TextCfg ('%s!%s!substring'%(key,k), False)
                    remove = CTK.ImageStock('del')
                    remove.bind('click', CTK.JS.Ajax (URL_APPLY, data={'%s!%s'%(key,k): ''},
                                                          complete = refresh.JS_to_refresh()))
                    table += [regex, subst, remove]

                submit = CTK.Submitter (URL_APPLY)
                submit += table

                self += CTK.Indenter (submit)

    def __init__ (self, key, key_entry):
        CTK.Container.__init__ (self)

        # List
        refresh = CTK.Refreshable ({'id': 'proxy_%s'%(key_entry)})
        refresh.register (lambda: self.Content(refresh, '%s!%s'%(key, key_entry)).Render())
        self += refresh

        # New
        new_regex  = CTK.TextCfg('tmp!new!%s!regex'%(key_entry), False, {'class': 'noauto'})
        new_subst  = CTK.TextCfg('tmp!new!%s!substring'%(key_entry), False, {'class': 'noauto'})
        add_button = CTK.SubmitterButton(_('Add'))

        table = CTK.Table()
        table.set_header(1)
        table += [CTK.RawHTML(x) for x in (_('Add RegEx'), _('Substitution'))]
        table += [new_regex, new_subst, add_button]

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Indenter (table)
        submit += CTK.Hidden ('tmp!new!%s!key'%(key_entry), key)
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
                    value = CTK.TextCfg ('%s!%s'%(key,k), False)
                    remove = CTK.ImageStock('del')
                    remove.bind('click', CTK.JS.Ajax (URL_APPLY, data={'%s!%s'%(key,k): ''},
                                                      complete = refresh.JS_to_refresh()))
                    table += [CTK.RawHTML(k), value, remove]

                submit = CTK.Submitter (URL_APPLY)
                submit += table

                self += CTK.Indenter (submit)

    def __init__ (self, key, key_entry):
        CTK.Container.__init__ (self)

        # List
        refresh = CTK.Refreshable ({'id': 'proxy_%s'%(key_entry)})
        refresh.register (lambda: self.Content(refresh, '%s!%s'%(key, key_entry)).Render())
        self += refresh

        # New
        new_regex  = CTK.TextCfg('tmp!new!%s!header'%(key_entry), False, {'class': 'noauto'})
        new_subst  = CTK.TextCfg('tmp!new!%s!value'%(key_entry), False, {'class': 'noauto'})
        add_button = CTK.SubmitterButton(_('Add'))

        table = CTK.Table()
        table.set_header(1)
        table += [CTK.RawHTML(x) for x in (_('Add Header Entry'), _('Value'))]
        table += [new_regex, new_subst, add_button]

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Indenter (table)
        submit += CTK.Hidden ('tmp!new!%s!key'%(key_entry), key)
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
                    remove.bind('click', CTK.JS.Ajax (URL_APPLY, data={'%s!%s'%(key,k): ''},
                                                          complete = refresh.JS_to_refresh()))
                    table += [CTK.RawHTML(CTK.cfg.get_val('%s!%s'%(key,k))), remove]

                submit = CTK.Submitter (URL_APPLY)
                submit += table

                self += CTK.Indenter (submit)

    def __init__ (self, key, key_entry):
        CTK.Container.__init__ (self)

        # List
        refresh = CTK.Refreshable ({'id': 'proxy_%s'%(key_entry)})
        refresh.register (lambda: self.Content(refresh, '%s!%s'%(key, key_entry)).Render())
        self += refresh

        # New
        new_hide  = CTK.TextCfg('tmp!new!%s!hide'%(key_entry), False, {'class': 'noauto'})
        add_button = CTK.SubmitterButton(_('Add'))

        table = CTK.Table()
        table.set_header(1)
        table += [CTK.RawHTML (_('Hide Header'))]
        table += [new_hide, add_button]

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Indenter (table)
        submit += CTK.Hidden ('tmp!new!%s!key'%(key_entry), key)
        submit.bind ('submit_success', refresh.JS_to_refresh())

        self += submit


class Plugin_proxy (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        kwargs['show_document_root'] = False
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        # Properties
        table = CTK.PropsTable()
        table.Add (_('Reuse connections'),         CTK.TextCfg ('%s!reuse_max'%(key), True), _(NOTE_REUSE_MAX))
        table.Add (_('Allow Keepalive'),           CTK.CheckCfgText('%s!in_allow_keepalive'%(key),  True,  _('Allow')),    _(NOTE_ALLOW_KEEPALIVE))
        table.Add (_('Preserve Host Header'),      CTK.CheckCfgText('%s!in_preserve_host'%(key),    False, _('Preserve')), _(NOTE_PRESERVE_HOST))
        table.Add (_('Preserve Server Header'),    CTK.CheckCfgText('%s!out_preserve_server'%(key), False, _('Preserve')), _(NOTE_PRESERVE_SERVER))
        table.Add (_('Use VServer error handler'), CTK.CheckCfgText('%s!vserver_errors'%(key),      False, _('Use it')),   _(NOTE_ERROR_HANDLER))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Reverse Proxy Settings')))
        self += CTK.Indenter (submit)

        # Request
        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Request')))

        self += CTK.Indenter (CTK.RawHTML ('<h3>%s</h3>' %(_('URL Rewriting'))))
        self += URL_Rewrite (key, 'in_rewrite_request')

        self += CTK.Indenter (CTK.RawHTML ('<h3>%s</h3>' %(_('Header Addition'))))
        self += Header_List (key, 'in_header_add')

        self += CTK.Indenter (CTK.RawHTML ('<h3>%s</h3>' %(_('Hide Headers'))))
        self += Header_Hide (key, 'in_header_hide')

        # Reply
        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Reply')))

        self += CTK.Indenter (CTK.RawHTML ('<h3>%s</h3>' %(_('URL Rewriting'))))
        self += URL_Rewrite (key, 'out_rewrite_request')

        self += CTK.Indenter (CTK.RawHTML ('<h3>%s</h3>' %(_('Header Addition'))))
        self += Header_List (key, 'out_header_add')

        self += CTK.Indenter (CTK.RawHTML ('<h3>%s</h3>' %(_('Hide Headers'))))
        self += Header_Hide (key, 'out_header_hide')

        # Balancer
        modul = CTK.PluginSelector('%s!balancer'%(key), trans_options(Cherokee.support.filter_available (BALANCERS)))
        table = CTK.PropsTable()
        table.Add (_("Balancer"), modul.selector_widget, _(Balancer.NOTE_BALANCER))

        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Back-end Servers')))
        self += CTK.Indenter (table)
        self += modul

CTK.publish ('^%s'%(URL_APPLY), commit, method="POST", validation=VALS)
