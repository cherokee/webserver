# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2013 Alvaro Lopez Ortega
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
import Icons
import Mime
import validations

from util import *
from consts import *
from configured import *

URL_BASE  = '/general'
URL_APPLY = '/general/apply'

NOTE_ADD_PORT    = N_('Defines the TCP port that the server will listen to.')
NOTE_ADD_IF      = N_('Optionally defines an interface to bind to. For instance: 127.0.0.1')
NOTE_IPV6        = N_('Set to enable the IPv6 support. The OS must support IPv6 for this to work.')
NOTE_TOKENS      = N_('This option allows to choose how the server identifies itself.')
NOTE_TIMEOUT     = N_('Time interval until the server closes inactive connections.')
NOTE_TLS         = N_('Which, if any, should be the TLS/SSL backend.')
NOTE_COLLECTORS  = N_('How the usage graphics should be generated.')
NOTE_POST_TRACKS = N_('How to track uploads/posts so its progress can be reported to the user.')
NOTE_USER        = N_('Changes the effective user. User names and IDs are accepted.')
NOTE_GROUP       = N_('Changes the effective group. Group names and IDs are accepted.')
NOTE_CHROOT      = N_('Jail the server inside the directory. Don\'t use it as the only security measure.')
NOTE_NO_BINDS    = N_('No ports to listen have been defined. By default the server will listen to TCP port 80 on all the network interfaces.')
NOTE_PORTS_INFO  = N_('Remember: The HTTP standard port is 80. The HTTPS port is 443.')

HELPS = [('config_general',    N_("General Configuration")),
         ('config_quickstart', N_("Configuration Quickstart"))]


VALIDATIONS = [
    ("server!ipv6",              validations.is_boolean),
    ("server!timeout",           validations.is_number),
    ("server!bind!.*!port",      validations.is_tcp_port),
    ("server!bind!.*!interface", validations.is_ip),
    ("server!bind!.*!tls",       validations.is_boolean),
    ("server!chroot",            validations.is_chroot_dir_exists),
    ("new_port",                 validations.is_tcp_port),
    ("new_if",                   validations.is_ip)
]

JS_SCROLL = """
function resize_cherokee_containers() {
   $('#mimetable tbody').height($(window).height() - 270);
   $('#body-general .ui-tabs-panel').height($(window).height() - 154);
}

$(document).ready(function() {
   resize_cherokee_containers();
   $(window).resize(function(){
       resize_cherokee_containers();
   });
});

"""


def is_new_bind (port, ip=None):
    if ip:
        ip   = validations.is_ip (ip)
        port = validations.is_tcp_port (port)
    else:
        port = validations.is_new_tcp_port (port)

    binds = [(CTK.cfg.get_val('server!bind!%s!port'%(x)),
              CTK.cfg.get_val('server!bind!%s!interface'%(x)))
             for x in CTK.cfg.keys ('server!bind')]

    if (port, ip) in binds:
        raise ValueError, _("Port/Interface combination already in use.")


def commit():
    # Add a new port
    port      = CTK.post.pop('new_port')
    interface = CTK.post.pop('new_if')

    if port:
        try:
            is_new_bind (port, interface)
        except ValueError, e:
            return { "ret": "error", "errors": { 'new_port': str(e),
                                                 'new_if':   str(e) }}

        # Look for the next entry
        pre = CTK.cfg.get_next_entry_prefix ('server!bind')

        # Add new port
        CTK.cfg['%s!port'%(pre)] = port
        if interface:
            CTK.cfg['%s!interface'%(pre)] = interface

        # Auto-set TLS/SSL flag
        if int(port) == 443:
            CTK.cfg['%s!tls'%(pre)] = "1"

    # Modifications
    return CTK.cfg_apply_post()


class NetworkWidget (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class':'network'})

        table = CTK.PropsTable()
        table.Add (_('IPv6'),             CTK.CheckCfgText('server!ipv6', True, _('Enabled')), _(NOTE_IPV6))
        table.Add (_('SSL/TLS back-end'), CTK.ComboCfg('server!tls', trans_options(Cherokee.support.filter_available(CRYPTORS))), _(NOTE_TLS))
        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Indenter(table)
        self += CTK.RawHTML ("<h2>%s</h2>" %(_('Support')))
        self += submit

        table = CTK.PropsTable()
        table.Add (_('Timeout (<i>secs</i>)'), CTK.TextCfg('server!timeout'), _(NOTE_TIMEOUT))
        table.Add (_('Server Tokens'),         CTK.ComboCfg('server!server_tokens', trans_options(PRODUCT_TOKENS)), _(NOTE_TOKENS))
        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Indenter(table)
        self += CTK.RawHTML ("<h2>%s</h2>" %(_('Network Behavior')))
        self += submit

        table = CTK.PropsTable()
        modul = CTK.PluginSelector('server!collector', trans_options(Cherokee.support.filter_available(COLLECTORS)))
        table.Add (_('Graphs Type'), modul.selector_widget, _(NOTE_COLLECTORS))
        self += CTK.RawHTML ("<h2>%s</h2>" %(_('Information Collector')))
        self += CTK.Indenter(table)
        self += CTK.Indenter(modul)

        table = CTK.PropsTable()
        modul = CTK.PluginSelector('server!post_track', trans_options(Cherokee.support.filter_available(POST_TRACKERS)))
        table.Add (_('Upload Tracking'), modul.selector_widget, _(NOTE_POST_TRACKS))
        self += CTK.RawHTML ("<h2>%s</h2>" %(_('Upload Tracking')))
        self += CTK.Indenter(table)
        self += CTK.Indenter(modul)


class PortsTable (CTK.Submitter):
    def __init__ (self, refreshable, **kwargs):
        CTK.Submitter.__init__ (self, URL_APPLY)

        table   = CTK.Table({'class':'ports'})
        binds   = CTK.cfg.keys('server!bind')
        has_tls = CTK.cfg.get_val('server!tls') != None

        # Skip if empty
        if not binds:
            self += CTK.Indenter (CTK.Notice('information', CTK.RawHTML (_(NOTE_NO_BINDS))))
            return

        # Header
        table[(1,1)] = [CTK.RawHTML(x) for x in (_('Port'), _('Bind to'), _('TLS'), '')]
        table.set_header (row=True, num=1)
        self += table

        # Entries
        n = 2
        for k in binds:
            pre = 'server!bind!%s'%(k)

            port   = CTK.TextCfg ('%s!port'%(pre),      False, {'size': 8})
            listen = CTK.TextCfg ('%s!interface'%(pre), True,  {'size': 45})

            if has_tls:
                tls = CTK.CheckCfgText ('%s!tls'%(pre), False, _('TLS/SSL port'))
            else:
                tls = CTK.CheckCfgText ('%s!tls'%(pre), False, _('TLS/SSL support disabled'), {'disabled': not has_tls})

            delete = None
            if len(binds) >= 2:
                delete = CTK.ImageStock('del')
                delete.bind('click', CTK.JS.Ajax (URL_APPLY,
                                                  data     = {pre: ''},
                                                  complete = refreshable.JS_to_refresh()))

            table[(n,1)] = [port, listen, tls, delete]
            n += 1


class PortsWidget (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        # List ports
        self.refresh = CTK.Refreshable({'id': 'general_ports'})
        self.refresh.register (lambda: PortsTable(self.refresh).Render())

        # Add new - dialog
        table = CTK.PropsTable()
        table.Add (_('Port'), CTK.TextCfg ('new_port', False, {'class':'noauto'}), _(NOTE_ADD_PORT))
        table.Add (_('Interface'), CTK.TextCfg ('new_if', True, {'class':'noauto'}), _(NOTE_ADD_IF))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        dialog = CTK.Dialog({'title': _('Add a new listening port'), 'autoOpen': False, 'draggable': False, 'width': 550 })
        dialog.AddButton (_("Cancel"), "close")
        dialog.AddButton (_("Add"),    submit.JS_to_submit())
        dialog += submit
        dialog += CTK.Notice (content = CTK.RawHTML('<p>%s</p>'%(_(NOTE_PORTS_INFO))))

        submit.bind ('submit_success', self.refresh.JS_to_refresh())
        submit.bind ('submit_success', dialog.JS_to_close())

        # Add new
        button = CTK.SubmitterButton ("%s…" %(_('Add new port')))
        button.bind ('click', dialog.JS_to_show())
        button_s = CTK.Submitter (URL_APPLY)
        button_s += button

        # Integration
        self += CTK.RawHTML ("<h2>%s</h2>" % (_('Listening to Ports')))
        self += CTK.Indenter (self.refresh)
        self += button_s
        self += dialog


class PermsWidget (CTK.Box):
    def __init__ (self):
        CTK.Box.__init__ (self, {'class':'permissions'})

        table = CTK.PropsAuto (URL_APPLY)
        self += CTK.RawHTML ("<h2>%s</h2>" %(_('Execution Permissions')))
        table.Add (_('User'),   CTK.TextCfg('server!user',   True),  _(NOTE_USER))
        table.Add (_('Group'),  CTK.TextCfg('server!group',  True), _(NOTE_GROUP))
        table.Add (_('Chroot'), CTK.TextCfg('server!chroot', True), _(NOTE_CHROOT))
        self += CTK.Indenter(table)


class Render:
    def __call__ (self):
        ports   = PortsWidget()
        network = NetworkWidget()
        network.bind ('submit_success', ports.refresh.JS_to_refresh())

        tabs = CTK.Tab()
        tabs.Add (_('Network'),         network)
        tabs.Add (_('Ports to listen'), ports)
        tabs.Add (_('Permissions'),     PermsWidget())
        tabs.Add (_('Icons'),           Icons.Icons_Widget())
        tabs.Add (_('Mime types'),      Mime.MIME_Widget())

        page = Page.Base (_("General"), body_id='general', helps=HELPS)
        page += CTK.RawHTML("<h1>%s</h1>" %(_('General Settings')))
        page += tabs
        page += CTK.RawHTML (js=JS_SCROLL)

        return page.Render()

CTK.publish ('^%s'%(URL_BASE), Render)
CTK.publish ('^%s'%(URL_APPLY), commit, validation=VALIDATIONS, method="POST")
