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
import Cherokee
import validations
import Handler

URL_APPLY = '/plugin/handler_cgi_base/apply'

NOTE_SCRIPT_ALIAS     = N_('Path to an executable that will be run with the CGI as parameter.')
NOTE_CHANGE_USER      = N_('Execute the CGI under its file owner user ID.')
NOTE_ERROR_HANDLER    = N_('Send errors exactly as they are generated.')
NOTE_CHECK_FILE       = N_('Check whether the file is in place.')
NOTE_PASS_REQ         = N_('Forward all the client headers to the CGI encoded as HTTP_*. headers.')
NOTE_XSENDFILE        = N_('Allow the use of the non-standard X-Sendfile header.')
NOTE_IOCACHE          = N_("Uses cache during file detection. Disable if directory contents change frequently. Enable otherwise.")
NOTE_X_REAL_IP        = N_('Whether the handler should read and use the X-Real-IP and X-Forwarded-For headers and use it in REMOTE_ADDR.')
NOTE_X_REAL_IP_ALL    = N_('Accept all the X-Real-IP and X-Forwarded-For headers. WARNING: Turn it on only if you are centain of what you are doing.')
NOTE_X_REAL_IP_ACCESS = N_('List of IP addresses and subnets that are allowed to send the X-Real-IP and X-Forwarded-For headers.')

HELPS = [('modules_handlers_cgi', "CGIs")]


def commit():
    new_name  = CTK.post.pop('tmp!new_name')
    new_value = CTK.post.pop('tmp!new_value')
    key       = CTK.post.pop('key')

    # New
    if new_name:
        CTK.cfg['%s!%s'%(key, new_name)] = new_value
        return CTK.cfg_reply_ajax_ok()

    # Modification
    return CTK.cfg_apply_post()


class PluginHandlerCGI (Handler.PluginHandler):
    class X_Real_IP (CTK.Container):
        def __init__ (self, refresh, key):
            CTK.Container.__init__ (self)

            x_real_ip     = int(CTK.cfg.get_val('%s!x_real_ip_enabled'    %(key), "0"))
            x_real_ip_all = int(CTK.cfg.get_val('%s!x_real_ip_access_all' %(key), "0"))

            table = CTK.PropsTable()
            table.Add (_('Read X-Real-IP'), CTK.CheckCfgText('%s!x_real_ip_enabled'%(key), False, _('Enabled')), _(NOTE_X_REAL_IP))
            if x_real_ip:
                table.Add (_('Don\'t check origin'), CTK.CheckCfgText('%s!x_real_ip_access_all'%(key), False, _('Enabled')), _(NOTE_X_REAL_IP_ALL))
                if not x_real_ip_all:
                    table.Add (_('Accept from Hosts'), CTK.TextCfg ('%s!x_real_ip_access'%(key), False), _(NOTE_X_REAL_IP_ACCESS))

            submit = CTK.Submitter (URL_APPLY)
            submit += table
            submit.bind ('submit_success', refresh.JS_to_refresh())
            self += submit

    class Environ (CTK.Container):
        def __init__ (self, refresh, key):
            CTK.Container.__init__ (self)

            envs = CTK.cfg.keys('%s!env' %(key))
            envs.sort()
            if not envs:
                return

            table = CTK.Table()
            table.set_header(1)
            table += [CTK.RawHTML(x) for x in (_('Name'), _('Value'))]

            for env in envs:
                pre   = '%s!env!%s'%(key, env)
                value = CTK.TextCfg (pre, False)
                remove = CTK.ImageStock('del')
                remove.bind('click', CTK.JS.Ajax (URL_APPLY, data = {pre: ''},
                                                  complete = refresh.JS_to_refresh()))
                table += [CTK.RawHTML(env), value, remove]

            submit = CTK.Submitter (URL_APPLY)
            submit += table
            submit.bind ('submit_success', refresh.JS_to_refresh())
            self += submit

    def __init__ (self, key, **kwargs):
        self.show_script_alias = kwargs.pop ('show_script_alias', True)
        self.show_change_uid   = kwargs.pop ('show_change_uid', True)

        Handler.PluginHandler.__init__ (self, key, **kwargs)

    def AddCommon (self):
        # Parent's AddComon
        Handler.PluginHandler.AddCommon (self)

        # Add CGI related stuff
        table = CTK.PropsTable()

        if self.show_script_alias:
            table.Add (_("Script Alias"), CTK.TextCfg('%s!script_alias'%(self.key), True),        _(NOTE_SCRIPT_ALIAS))
        if self.show_change_uid:
            table.Add (_("Change UID"),   CTK.CheckCfgText('%s!change_user'%(self.key), False, _('Change')), _(NOTE_CHANGE_USER))

        table.Add (_('Error handler'),        CTK.CheckCfgText('%s!error_handler'%(self.key),    True,  _('Enabled')), _(NOTE_ERROR_HANDLER))
        table.Add (_('Check file'),           CTK.CheckCfgText('%s!check_file'%(self.key),       True,  _('Enabled')), _(NOTE_CHECK_FILE))
        table.Add (_('Pass Request Headers'), CTK.CheckCfgText('%s!pass_req_headers'%(self.key), True,  _('Enabled')), _(NOTE_PASS_REQ))
        table.Add (_('Allow X-Sendfile'),     CTK.CheckCfgText('%s!xsendfile'%(self.key),        False, _('Enabled')), _(NOTE_XSENDFILE))
        table.Add (_('Use I/O cache'),        CTK.CheckCfgText('%s!iocache'%(self.key),          True,  _('Enabled')), _(NOTE_IOCACHE))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Indenter (table)

        self += CTK.RawHTML ("<h2>%s</h2>" %(_('Common CGI Options')))
        self += submit

        # X-Real-IP
        refresh = CTK.Refreshable ({'id': 'x_real_ip'})
        refresh.register (lambda: self.X_Real_IP(refresh, self.key).Render())

        self += CTK.RawHTML ("<h2>%s</h2>" % (_('Fixed Authentication List')))
        self += CTK.Indenter (refresh)

        # Environment
        refresh = CTK.Refreshable ({'id': 'cgi_environ_list'})
        refresh.register (lambda: self.Environ(refresh, self.key).Render())

        self += CTK.RawHTML ("<h2>%s</h2>" % (_('Custom Environment Variables')))
        self += CTK.Indenter (refresh)

        new_name  = CTK.TextField ({'name': 'tmp!new_name',  'class': 'noauto'})
        new_value = CTK.TextField ({'name': 'tmp!new_value', 'class': 'noauto'})
        new_add   = CTK.SubmitterButton (_('Add'))

        table = CTK.Table()
        table.set_header(1)
        table += [CTK.RawHTML(x) for x in (_('Name'), _('Value'))]
        table += [new_name, new_value, new_add]

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('key', '%s!env'%(self.key))
        submit += table
        submit.bind ('submit_success', refresh.JS_to_refresh() + new_name.JS_to_clean() + new_value.JS_to_clean())

        self += CTK.RawHTML ("<h3>%s</h3>" % (_('Add New Custom Environment Variable')))
        self += CTK.Indenter(submit)

        # Publish
        VALS = [("%s!script_alias"%(self.key), validations.is_path)]
        CTK.publish ('^%s'%(URL_APPLY), commit, validation=VALS, method="POST")
