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
import Page
import Cherokee
import validations

from util import *
from consts import *
from configured import *

URL_BASE  = '/advanced'
URL_APPLY = '/advanced/apply'

KNOWN_TLS_PROTOCOLS = [
    ('SSLv2',   'SSL version 2'),
    ('SSLv3',   'SSL version 3'),
    ('TLSv1',   'TLS version 1'),
    ('TLSv1.1', 'TLS version 1.1'),
    ('TLSv1.2', 'TLS version 1.2'),
    ('TLSv1.3', 'TLS version 1.3')
]

# Keep aligned with cherokee_cryptor_init_base()
TLS_PROTOCOL_CHECKBOX_DEFAULT = {
    'SSLv2':    True,
    'SSLv3':    True,
    'TLSv1':   False,
    'TLSv1.1': False,
    'TLSv1.2': False,
    'TLSv1.3': False
}

TLS_WARNING = N_("""<p><b>WARNING</b>: The SSL/TLS back-end supports more recent
versions of TLS protocols, which are not recognized by Cherokee. Please check
for latest Cherokee updates or report this issue. As a temporary workaround please
set 'Max. TLS protocol version' to the maximum TLS protocol version supported by
Cherokee.</p><p>Following unknown TLS protocols have been found:""")

VALIDATIONS = [
    ("server!fdlimit",                validations.is_positive_int),
    ("server!pid_file",               validations.can_create_file),
    ("server!sendfile_min",           validations.is_positive_int),
    ("server!sendfile_max",           validations.is_positive_int),
    ('server!panic_action',           validations.is_local_file_exists),
    ('server!listen_queue',           validations.is_positive_int),
    ('server!max_connection_reuse',   validations.is_positive_int),
    ('server!log_flush_lapse',        validations.is_positive_int),
    ('server!keepalive_max_requests', validations.is_positive_int),
    ("server!keepalive$",             validations.is_boolean),
    ("server!thread_number",          validations.is_positive_int),
    ("server!nonces_cleanup_lapse",   validations.is_positive_int),
    ("server!iocache$",               validations.is_boolean),
    ("server!iocache!max_size",       validations.is_positive_int_4_multiple),
    ("server!iocache!min_file_size",  validations.is_positive_int),
    ("server!iocache!max_file_size",  validations.is_positive_int),
    ("server!iocache!lasting_stat",   validations.is_positive_int),
    ("server!iocache!lasting_mmap",   validations.is_positive_int),
    ("server!tls!protocol!SSLv2",     validations.is_boolean),
    ("server!tls!protocol!SSLv3",     validations.is_boolean),
    ("server!tls!protocol!TLSv1",     validations.is_boolean),
    ("server!tls!protocol!TLSv1_1",   validations.is_boolean),
    ("server!tls!protocol!TLSv1_2",   validations.is_boolean),
    ("server!tls!protocol!TLSv1_3",   validations.is_boolean),
    ("server!tls!timeout_handshake",  validations.is_positive_int),
    ("server!tls!dh_param512",        validations.is_local_file_exists),
    ("server!tls!dh_param1024",       validations.is_local_file_exists),
    ("server!tls!dh_param2048",       validations.is_local_file_exists),
    ("server!tls!dh_param4096",       validations.is_local_file_exists),
]

WARNING = N_("""<p><b>WARNING</b>: This section contains advanced
configuration parameters. Changing things is not recommended unless
you really know what you are doing.</p>""")

NOTE_THREAD       = N_('Defines which thread policy the OS should apply to the server.')
NOTE_THREAD_NUM   = N_('If empty, Cherokee will calculate a default number.')
NOTE_FD_NUM       = N_('It defines how many file descriptors the server should handle. Default is the number showed by ulimit -n')
NOTE_POLLING      = N_('Allows to choose the internal file descriptor polling method.')
NOTE_SENDFILE_MIN = N_('Minimum size of a file to use sendfile(). Default: 32768 Bytes.')
NOTE_SENDFILE_MAX = N_('Maximum size of a file to use sendfile(). Default: 2 GB.')
NOTE_PANIC_ACTION = N_('Name a program that will be called if, by some reason, the server fails. Default: <em>cherokee-panic</em>.')
NOTE_PID_FILE     = N_('Path of the PID file. If empty, the file will not be created.')
NOTE_LISTEN_Q     = N_('Max. length of the incoming connection queue.')
NOTE_REUSE_CONNS  = N_('Set the number of how many internal connections can be held for reuse by each thread. Default: 20.')
NOTE_FLUSH_TIME   = N_('Sets the number of seconds between log consolidations (flushes). Default: 10 seconds.')
NOTE_NONCES_TIME  = N_('Time lapse (in seconds) between Nonce cache clean ups.')
NOTE_KEEPALIVE    = N_('Enables the server-wide keep-alive support. It increases the performance. It is usually set on.')
NOTE_KEEPALIVE_RS = N_('Maximum number of HTTP requests that can be served by each keepalive connection.')
NOTE_CHUNKED      = N_('Allows the server to use Chunked encoding to try to keep Keep-Alive enabled.')
NOTE_IO_ENABLED   = N_('Activate or deactivate the I/O cache globally.')
NOTE_IO_SIZE      = N_('Number of pages that the cache should handle.')
NOTE_IO_MIN_SIZE  = N_('Files under this size (in bytes) will not be cached.')
NOTE_IO_MAX_SIZE  = N_('Files over this size (in bytes) will not be cached.')
NOTE_IO_LAST_STAT = N_('How long (in seconds) the file information should last cached without refreshing it.')
NOTE_IO_LAST_MMAP = N_('How long (in seconds) the file content should last cached.')
NOTE_DH512        = N_('Path to a Diffie Hellman (DH) parameters PEM file: 512 bits.')
NOTE_DH1024       = N_('Path to a Diffie Hellman (DH) parameters PEM file: 1024 bits.')
NOTE_DH2048       = N_('Path to a Diffie Hellman (DH) parameters PEM file: 2048 bits.')
NOTE_DH4096       = N_('Path to a Diffie Hellman (DH) parameters PEM file: 4096 bits.')
NOTE_TLS_NA       = N_('Your Cherokee Web Server does not support SSL/TLS encryption.')
NOTE_TLS_TIMEOUT  = N_('Timeout for the TLS/SSL handshake. Default: 15 seconds.')
NOTE_TLS_SSLv2    = N_('Beware: it is vulnerable. You should disable SSLv2.')
NOTE_TLS_SSLv3    = N_('Beware: it is vulnerable. You should disable SSLv3.')
NOTE_TLS_TLSv1    = N_('TLSv1 is deprecated')
NOTE_TLS_TLSv1_1  = N_('TLSv1.1 is deprecated')
NOTE_TLS_TLSv1_2  = N_('  ')
NOTE_TLS_TLSv1_3  = N_('  ')
NOTE_TLS_DISABLE  = N_('The following options work in combination with above min/max TLS protocol version settings. '
                       'They are deprecated, only set the minimum and maximum supported protocol versions instead.')
NOTE_TLS_MIN      = N_('Minimum required TLS protocol version from clients (Default: auto-configured by libssl)')
NOTE_TLS_MAX      = N_('Maximum supported TLS protocol version (Default: auto-configured by libssl)')

TLS_PROTOCOL_NOTES = {
    'SSLv2':   NOTE_TLS_SSLv2,
    'SSLv3':   NOTE_TLS_SSLv3,
    'TLSv1':   NOTE_TLS_TLSv1,
    'TLSv1.1': NOTE_TLS_TLSv1_1,
    'TLSv1.2': NOTE_TLS_TLSv1_2,
    'TLSv1.3': NOTE_TLS_TLSv1_3
}

HELPS = [('config_advanced', N_('Advanced'))]

JS_SCROLL = """
function resize_cherokee_containers() {
   $('#body-advanced .ui-tabs-panel').height($(window).height() - 154);
}

$(document).ready(function() {
   resize_cherokee_containers();
   $(window).resize(function(){
       resize_cherokee_containers();
   });
});

"""


class ConnectionsWidget (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        table = CTK.PropsAuto(URL_APPLY)
        table.Add (_('Keep Alive'),         CTK.CheckCfgText('server!keepalive', True, _("Allowed")), _(NOTE_KEEPALIVE))
        table.Add (_('Max keepalive reqs'), CTK.TextCfg('server!keepalive_max_requests'), _(NOTE_KEEPALIVE_RS))
        table.Add (_('Chunked Encoding'),   CTK.CheckCfgText('server!chunked_encoding', True, _("Allowed")), _(NOTE_CHUNKED))
        table.Add (_('Polling Method'),     CTK.ComboCfg('server!poll_method', trans_options(Cherokee.support.filter_polling_methods(POLL_METHODS))), _(NOTE_POLLING))
        table.Add (_('Sendfile min size'),  CTK.TextCfg('server!sendfile_min', True), _(NOTE_SENDFILE_MIN))
        table.Add (_('Sendfile max size'),  CTK.TextCfg('server!sendfile_max', True), _(NOTE_SENDFILE_MAX))

        self += CTK.RawHTML ("<h2>%s</h2>" %(_('Connections')))
        self += CTK.Indenter(table)

class ResourcesWidget (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        table = CTK.PropsAuto(URL_APPLY)
        table.Add (_('Thread Number'),          CTK.TextCfg('server!thread_number', True), _(NOTE_THREAD_NUM))
        table.Add (_('Thread Policy'),          CTK.ComboCfg('server!thread_policy', trans_options(THREAD_POLICY)), _(NOTE_THREAD))
        table.Add (_('File descriptors'),       CTK.TextCfg('server!fdlimit',              True), _(NOTE_FD_NUM))
        table.Add (_('Listening queue length'), CTK.TextCfg('server!listen_queue',         True), _(NOTE_LISTEN_Q))
        table.Add (_('Reuse connections'),      CTK.TextCfg('server!max_connection_reuse', True), _(NOTE_REUSE_CONNS))
        table.Add (_('Log flush time'),         CTK.TextCfg('server!log_flush_lapse',      True), _(NOTE_FLUSH_TIME))
        table.Add (_('Nonces clean up time'),   CTK.TextCfg('server!nonces_cleanup_lapse', True), _(NOTE_NONCES_TIME))

        self += CTK.RawHTML ("<h2>%s</h2>" %(_('Resources')))
        self += CTK.Indenter(table)

class IOCacheWidget (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        table = CTK.PropsAuto(URL_APPLY)
        table.Add (_('Status'),        CTK.CheckCfgText('server!iocache', True, _('Enabled')), _(NOTE_IO_ENABLED))
        table.Add (_('Max pages'),     CTK.TextCfg('server!iocache!max_size',      True), _(NOTE_IO_SIZE))
        table.Add (_('File Min Size'), CTK.TextCfg('server!iocache!min_file_size', True), _(NOTE_IO_MIN_SIZE))
        table.Add (_('File Max Size'), CTK.TextCfg('server!iocache!max_file_size', True), _(NOTE_IO_MAX_SIZE))
        table.Add (_('Lasting: stat'), CTK.TextCfg('server!iocache!lasting_stat',  True), _(NOTE_IO_LAST_STAT))
        table.Add (_('Lasting: mmap'), CTK.TextCfg('server!iocache!lasting_mmap',  True), _(NOTE_IO_LAST_MMAP))

        self += CTK.RawHTML ("<h2>%s</h2>" %(_('I/O cache')))
        self += CTK.Indenter(table)

class SpecialFilesWidget (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        table = CTK.PropsAuto(URL_APPLY)
        table.Add (_('Panic action'), CTK.TextCfg('server!panic_action', True), _(NOTE_PANIC_ACTION))
        table.Add (_('PID file'),     CTK.TextCfg('server!pid_file',     True), _(NOTE_PID_FILE))

        self += CTK.RawHTML ("<h2>%s</h2>" %(_('Special Files')))
        self += CTK.Indenter(table)

class TLSWidget (CTK.Container):
    @staticmethod
    def check_for_tls_protocol_versions (self):
        available_tls_protocols = {}
        unknown_tls_protocols = []
        tls_option_list = TLSPROTOCOLS.split(',')
        if len(tls_option_list) > 1:
            for protocol in tls_option_list:
                for known_protocol in KNOWN_TLS_PROTOCOLS:
                    if known_protocol[0] == protocol:
                        available_tls_protocols[protocol] = known_protocol[1]
                        break
                else:
                     unknown_tls_protocols.append(protocol)
        return available_tls_protocols, unknown_tls_protocols

    @staticmethod
    def add_tls_version_config (self, table):
        props = {}
        props = {'mode': 'inverse'}
        props_disable = props.copy()
        props_disable["disabled"] = 1
        for protocol in KNOWN_TLS_PROTOCOLS:
            if self.available_tls_protocols.has_key(protocol[0]):
                table.Add ('%s%s' % (_('Disable '), self.available_tls_protocols[protocol[0]]), CTK.CheckCfgText('server!tls!protocol!' + protocol[0].replace('.','_'), TLS_PROTOCOL_CHECKBOX_DEFAULT[protocol[0]], _("Disable"), props), TLS_PROTOCOL_NOTES[protocol[0]])
            else:
                table.Add ('%s%s' % (_('Disable '), protocol[1]), CTK.CheckCfgText('server!tls!protocol!' + protocol[0].replace('.','_'),  True, _("Protocol deacrivated") if protocol[0].startswith('SSLv') else _("Protocol not supported"), props_disable), TLS_PROTOCOL_NOTES[protocol[0]])

    @staticmethod
    def add_tls_version_disable_section (self):
        props = {}
        props = {'mode': 'inverse'}
        props_disable = props.copy()
        props_disable["disabled"] = 1
        table = CTK.PropsAuto(URL_APPLY)
        for protocol in KNOWN_TLS_PROTOCOLS:
            if self.available_tls_protocols.has_key(protocol[0]):
                table.Add ('%s%s' % (_('Disable '), self.available_tls_protocols[protocol[0]]), CTK.CheckCfgText('server!tls!protocol!' + protocol[0].replace('.','_'), TLS_PROTOCOL_CHECKBOX_DEFAULT[protocol[0]], _("Disable"), props), "")
            else:
                table.Add ('%s%s' % (_('Disable '), protocol[1]), CTK.CheckCfgText('server!tls!protocol!' + protocol[0].replace('.','_'),  True, _("Protocol deactivated"), props_disable), "")

        self += CTK.RawHTML ("<h3>%s</h3>" %(_('Turn off selected TLS Protocols (Deprecated)')))
        tip = '%s' %(_(NOTE_TLS_DISABLE))
        props = {'class': 'dialog-information-slim'}
        self += CTK.Indenter (CTK.Notice ('information', CTK.RawHTML(tip), props))
        self += CTK.Indenter(table)

    def __init__ (self):
        CTK.Container.__init__ (self)
        tls_min_protocols = [('auto',    N_('auto-configured'))]
        tls_max_protocols = [('auto',    N_('auto-configured'))]

        if MINMAXTLS == True and KNOWN_TLS_PROTOCOLS[0][0] == 'SSLv2':
            # Staring with OpenSSL 1.1.1, i. e. MINMAXTLS = True SSLv2 has been removed
            KNOWN_TLS_PROTOCOLS.pop(0)
        self.available_tls_protocols, self.unknown_tls_protocols = TLSWidget.check_for_tls_protocol_versions (self)

        table = CTK.PropsAuto(URL_APPLY)
        if MINMAXTLS == False:
            TLSWidget.add_tls_version_config (self, table)
        else:
            for protocol in KNOWN_TLS_PROTOCOLS:
                if self.available_tls_protocols.has_key(protocol[0]):
                    tls_min_protocols.append((protocol[0], N_(self.available_tls_protocols[protocol[0]])))
            table.Add (_('Min. TLS protocol version'), CTK.ComboCfg('server!tls!protocol!min', trans_options(tls_min_protocols)), _(NOTE_TLS_MIN))
            KNOWN_TLS_PROTOCOLS.reverse()
            for protocol in KNOWN_TLS_PROTOCOLS:
                if self.available_tls_protocols.has_key(protocol[0]):
                    tls_max_protocols.append((protocol[0], N_(self.available_tls_protocols[protocol[0]])))
            table.Add (_('Max. TLS protocol version'), CTK.ComboCfg('server!tls!protocol!max', trans_options(tls_max_protocols)), _(NOTE_TLS_MAX))
        table.Add (_('Handshake Timeout'),         CTK.TextCfg('server!tls!timeout_handshake', True), _(NOTE_TLS_TIMEOUT))
        table.Add (_('DH parameters: 512 bits'),   CTK.TextCfg('server!tls!dh_param512',  True), _(NOTE_DH512))
        table.Add (_('DH parameters: 1024 bits'),  CTK.TextCfg('server!tls!dh_param1024', True), _(NOTE_DH1024))
        table.Add (_('DH parameters: 2048 bits'),  CTK.TextCfg('server!tls!dh_param2048', True), _(NOTE_DH2048))
        table.Add (_('DH parameters: 4096 bits'),  CTK.TextCfg('server!tls!dh_param4096', True), _(NOTE_DH4096))
        self += CTK.RawHTML ("<h2>%s</h2>" %(_('TLS')))
        if len(self.unknown_tls_protocols) > 0:
            tip = '%s %s</p>' % (_(TLS_WARNING), join(self.unknown_tls_protocols))
            props = {'class': 'dialog-error-slim'}
            self += CTK.Notice ('error', CTK.RawHTML(tip), props)
        if len(self.available_tls_protocols) == 0:
            self += CTK.RawHTML ("<p>%s</p>" %(_(NOTE_TLS_NA)))
        else:
            self += CTK.Indenter(table)

        if MINMAXTLS == True:
            KNOWN_TLS_PROTOCOLS.reverse()
            TLSWidget.add_tls_version_disable_section (self)

class Render:
    def __call__ (self):
        tabs = CTK.Tab()
        tabs.Add (_('Connections'),   ConnectionsWidget())
        tabs.Add (_('Resources'),     ResourcesWidget())
        tabs.Add (_('I/O cache'),     IOCacheWidget())
        tabs.Add (_('Special Files'), SpecialFilesWidget())
        tabs.Add (_('TLS'),           TLSWidget())

        notice = CTK.Notice('warning')
        notice += CTK.RawHTML(_(WARNING))

        page = Page.Base(_('Advanced'), body_id='advanced', helps=HELPS)
        page += CTK.RawHTML("<h1>%s</h1>" %(_('Advanced Configuration')))
        page += notice
        page += tabs
        page += CTK.RawHTML (js=JS_SCROLL)

        return page.Render()


CTK.publish ('^%s'%(URL_BASE), Render)
CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, validation=VALIDATIONS, method="POST")
