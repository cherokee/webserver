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
import copy
import Page
import Cherokee
import validations

import os
import re

from util import *
from consts import *
from configured import *

from Rule import Rule
from CTK.util import find_copy_name

URL_BASE  = '/vserver/content'
URL_APPLY = '/vserver/content/apply'

NOTE_NICKNAME         = N_('Nickname for the virtual server.')
NOTE_CERT             = N_('This directive points to the PEM-encoded Certificate file for the server (Full path to the file)')
NOTE_CERT_KEY         = N_('PEM-encoded Private Key file for the server (Full path to the file)')
NOTE_CA_LIST          = N_('File containing the trusted CA certificates, utilized for checking the client certificates (Full path to the file)')
NOTE_CIPHERS          = N_('Ciphers that TLS/SSL is allowed to use. <a target="_blank" href="http://www.openssl.org/docs/apps/ciphers.html">Reference</a>. (Default enables Forward Secrecy).')
NOTE_CIPHER_SERVER_PREFERENCE = N_('The cipher sequence that is specified by the server should have preference over the client preference. (Default: True).')
NOTE_COMPRESSION      = N_('Explicitly enable or disable serverside compression support. (Default: Disabled).')
NOTE_DH_LENGTH        = N_('Explicitely sets the Diffie-Hellman parameters length. (Default: Let openssl choose).')
NOTE_CLIENT_CERTS     = N_('Skip, Tolerate, Accept or Require client certificates.')
NOTE_VERIFY_DEPTH     = N_('Limit up to which depth certificates in a chain are used during the verification procedure (Default: 1)')
NOTE_ERROR_HANDLER    = N_('Allows the selection of how to generate the error responses.')
NOTE_PERSONAL_WEB     = N_('Directory inside the user home directory to use as root web directory. Disabled if empty.')
NOTE_DISABLE_PW       = N_('The personal web support is currently turned on.')
NOTE_ADD_DOMAIN       = N_('Adds a new domain name. Wildcards are allowed in the domain name.')
NOTE_DOCUMENT_ROOT    = N_('Virtual Server root directory.')
NOTE_DIRECTORY_INDEX  = N_('List of name files that will be used as directory index. Eg: <em>index.html,index.php</em>.')
NOTE_MAX_UPLOAD_SIZE  = N_('The maximum size, in bytes, for POST uploads. (Default: unlimited)')
NOTE_KEEPALIVE        = N_('Whether this virtual server is allowed to use Keep-alive (Default: yes)')
NOTE_DISABLE_LOG      = N_('The Logging is currently enabled.')
NOTE_LOGGERS          = N_('Logging format. Apache compatible is highly recommended here.')
NOTE_ACCESSES         = N_('Back-end used to store the log accesses.')
NOTE_ERRORS           = N_('Back-end used to store the log errors.')
NOTE_ACCESSES_ERRORS  = N_('Back-end used to store the log accesses and errors.')
NOTE_WRT_FILE         = N_('Full path to the file where the information will be saved.')
NOTE_WRT_EXEC         = N_('Path to the executable that will be invoked on each log entry.')
NOTE_X_REAL_IP        = N_('Whether the logger should read and use the X-Real-IP and X-Forwarded-For headers (send by reverse proxies).')
NOTE_X_REAL_IP_ALL    = N_('Accept all the X-Real-IP and X-Forwarded-For headers. WARNING: Turn it on only if you are centain of what you are doing.')
NOTE_X_REAL_IP_ACCESS = N_('List of IP addresses and subnets that are allowed to send X-Real-IP and X-Forwarded-For headers.')
NOTE_EVHOST           = N_('How to support the "Advanced Virtual Hosting" mechanism. (Default: off)')
NOTE_LOGGER_TEMPLATE  = N_('The following variables are accepted: ')
NOTE_MATCHING_METHOD  = N_('Allows the selection of domain matching method.')
NOTE_COLLECTOR        = N_('Whether or not it should collected statistics about the traffic of this virtual server.')
NOTE_UTC_TIME         = N_('Time standard to use in the log file entries.')
NOTE_INDEX_USAGE      = N_('Remember that only "File Exists" rules and "List & Send" handlers use the Directory Indexes setting.')
NOTE_MATCH_NICK       = N_('Use this nickname as an additional host name for this virtual server (Default: yes)')
NOTE_HSTS             = N_('Enforce HTTPS by using the HTTP Strict Transport Security.')
NOTE_HSTS_MAXAGE      = N_("How long the client's browser should remember the forced HTTPS (in seconds).")
NOTE_HSTS_SUBDOMAINS  = N_("Should HSTS be used in all the subdomains of this virtual server (Default: yes).")

DEFAULT_HOST_NOTE     = N_("<p>The 'default' virtual server matches all the domain names.</p>")

CUSTOM_MACROS_LIST = [
    ('ip_remote',          N_("Remote IP address")),
    ('ip_local',           N_("Local IP address")),
    ('protocol',           N_("Request Protocol")),
    ('transport',          N_("Transport type: http or https")),
    ('port_server',        N_("Port of the server serving the request")),
    ('query_string',       N_("The query string, if exists")),
    ('request_first_line', N_("First line of HTTP request")),
    ('status',             N_("Response status code")),
    ('now',                N_("Time: in common log format time format")),
    ('time_secs',          N_("Time: seconds since Epoch")),
    ('time_msecs',         N_("Time: milliseconds since Epoch")),
    ('user_remote',        N_("Remote user (authentication)")),
    ('request',            N_("URL path requested")),
    ('request_original',   N_("URL path requested before any rewrite")),
    ('vserver_name',       N_("Virtual Server nick name")),
    ('vserver_name_req',   N_("Requested host (Host: header)")),
    ('response_size',      N_("Size of the response in bytes")),
    ('http_host',          N_("'Host:' header of the request")),
    ('http_referrer',      N_("'Referrer:' header of the request")),
    ('http_user_agent',    N_("'User-Agent:' header of the request")),
    ('http_cookie',        N_("'Cookie:' header of the request")),
]


JS_TR_ODD = """
$('#rules-table tr:odd').addClass('trodd');
"""

JS_TR_INACTIVE = """
$("#rules-table tr td .inactive-rule").parent().parent().addClass('inactive-rule');
"""

def validation_ca_list (text):
    "Empty or local_file_exists"
    if not text:
        return text
    text = validations.is_local_file_exists (text)
    return text

VALIDATIONS = [
    ("vserver![\d]+!user_dir",                   validations.is_safe_id),
    ("vserver![\d]+!document_root",              validations.is_dev_null_or_local_dir_exists),
    ("vserver![\d]+!post_max_len",               validations.is_positive_int),
    ("vserver![\d]+!ssl_certificate_file",       validations.is_local_file_exists),
    ("vserver![\d]+!ssl_certificate_key_file",   validations.is_valid_certkey),
    ("vserver![\d]+!ssl_ca_list_file",           validation_ca_list),
    ("vserver![\d]+!ssl_verify_depth",           validations.is_positive_int),
    ("vserver![\d]+!error_writer!filename",      validations.can_create_file),
    ("vserver![\d]+!error_writer!command",       validations.is_local_file_exists),
    ("vserver![\d]+!logger!.+?!filename",        validations.can_create_file),
    ("vserver![\d]+!logger!.+?!command",         validations.is_local_file_exists),
    ("vserver![\d]+!logger!x_real_ip_access$",   validations.is_ip_or_netmask_list),
]

HELPS = [
    ('config_virtual_servers', N_("Virtual Servers")),
    ('modules_loggers',        N_("Loggers")),
    ('cookbook_ssl',           N_("SSL cookbook"))
]

BEHAVIOR_TAGS = [
    (N_('Rule Match'),           N_('Match')),
    (N_('Assigned Handler'),     N_('Handler')),
    (N_('Authentication'),       N_('Auth')),
    (N_('Custom Document Root'), N_('Root')),
    (N_('Only HTTPS'),           N_('Secure')),
    (N_('Encoders'),             N_('Enc')),
    (N_('Cacheable'),            N_('Cache')),
    (N_('Expiration'),           N_('Exp')),
    (N_('Connection Timeout'),   N_('Timeout')),
    (N_('Traffic Shaping'),      N_('Shaping')),
    (N_('Logging Enabled'),      N_('Log')),
    (N_('Rule is Final'),        N_('Final')),
]


def commit_clone():
    num = re.findall(r'^%s/([\d]+)/clone$'%(URL_BASE), CTK.request.url)[0]
    next = CTK.cfg.get_next_entry_prefix ('vserver')

    orig  = CTK.cfg.get_val ('vserver!%s!nick'%(num))
    names = [CTK.cfg.get_val('vserver!%s!nick'%(x)) for x in CTK.cfg.keys('vserver')]
    new_nick = find_copy_name (orig, names)

    CTK.cfg.clone ('vserver!%s'%(num), next)
    CTK.cfg['%s!nick' %(next)] = new_nick
    return CTK.cfg_reply_ajax_ok()


class HostMatchWidget_GeneralProps (CTK.Container):
    def __init__ (self, vsrv_num, refresh):
        CTK.Container.__init__ (self)

        pre       = "vserver!%s" %(vsrv_num)
        url_apply = "%s/%s" %(URL_APPLY, vsrv_num)

        if CTK.cfg.get_val ('%s!match'%(pre)):
            table = CTK.PropsAuto (url_apply)
            table.Add (_('Match nickname'), CTK.CheckCfgText('%s!match!nick'%(pre), True, _('Match')), _(NOTE_MATCH_NICK))
            submit = CTK.Submitter (url_apply)
            submit += CTK.Indenter (table)

            self += CTK.RawHTML ('<h2>%s</h2>' %(_('Host Match')))
            self += submit


class HostMatchWidget (CTK.Container):
    def __init__ (self, vsrv_num):
        CTK.Container.__init__ (self)

        pre        = "vserver!%s" %(vsrv_num)
        url_apply  = "%s/%s" %(URL_APPLY, vsrv_num)
        is_default = CTK.cfg.get_lowest_entry("vserver") == int(vsrv_num)
        is_complex = CTK.cfg.get_val ('%s!match'%(pre)) == 'v_or'

        # Default Virtual Server
        if is_default:
            notice  = CTK.Notice()
            notice += CTK.RawHTML(_(DEFAULT_HOST_NOTE))
            self += notice
            return

        # Complex matches
        if is_complex:
            # General Properties
            refresh = CTK.Refreshable ({'id': 'vserver_match_props'})
            refresh.register (lambda: HostMatchWidget_GeneralProps(vsrv_num, refresh).Render())
            self += refresh

            # Left side rule
            table = CTK.PropsAuto (url_apply)
            modul = CTK.PluginSelector ('%s!match!left'%(pre), trans_options(Cherokee.support.filter_available(VRULES)), vsrv_num=vsrv_num)
            table.Add ('%s 1'%(_('Method')), modul.selector_widget, _(NOTE_MATCHING_METHOD), False)
            self += CTK.Indenter (table)
            self += modul

            submit = CTK.Submitter ('%s/%s/vmatch'%(URL_APPLY, vsrv_num))
            submit += CTK.Hidden ('op', 'del_1')
            submit.bind ('submit_success', refresh.JS_to_refresh())

            remove_1 = CTK.Link (None, CTK.RawHTML(_('Remove criteria')))
            remove_1.bind ('click', submit.JS_to_submit())
            self += submit
            self += remove_1

            # Right side rule
            table = CTK.PropsAuto (url_apply)
            modul = CTK.PluginSelector ('%s!match!right'%(pre), trans_options(Cherokee.support.filter_available(VRULES)), vsrv_num=vsrv_num)
            table.Add ('%s 2'%(_('Method')), modul.selector_widget, _(NOTE_MATCHING_METHOD), False)
            self += CTK.Indenter (table)
            self += modul

            submit = CTK.Submitter ('%s/%s/vmatch'%(URL_APPLY, vsrv_num))
            submit += CTK.Hidden ('op', 'del_2')
            submit.bind ('submit_success', refresh.JS_to_refresh())

            remove_2 = CTK.Link (None, CTK.RawHTML(_('Remove criteria')))
            remove_2.bind ('click', submit.JS_to_submit())
            self += submit
            self += remove_2

        # Simple match
        else:
            # General Properties
            refresh = CTK.Refreshable ({'id': 'vserver_match_props'})
            refresh.register (lambda: HostMatchWidget_GeneralProps(vsrv_num, refresh).Render())
            self += refresh

            # Match module
            table = CTK.PropsAuto (url_apply)
            modul = CTK.PluginSelector ('%s!match'%(pre), trans_options(Cherokee.support.filter_available(VRULES)), vsrv_num=vsrv_num)
            modul.selector_widget.bind ('changed', refresh.JS_to_refresh())
            table.Add (_('Method'), modul.selector_widget, _(NOTE_MATCHING_METHOD), False)

            self += CTK.RawHTML ('<h2>%s</h2>' %(_('How to match this Virtual Server')))
            self += CTK.Indenter (table)
            self += modul

            submit = CTK.Submitter ('%s/%s/vmatch'%(URL_APPLY, vsrv_num))
            submit += CTK.Hidden ('op', 'or')

            link_add = CTK.Link (None, CTK.RawHTML (_('Add an additional matching criteria')))
            link_add.bind ('click', submit.JS_to_submit())

            self += submit
            self += link_add


def HostMatchWidget_Apply():
    def move (old, new):
        val = CTK.cfg.get_val(old)
        tmp = copy.deepcopy (CTK.cfg[old])
        del (CTK.cfg[old])

        CTK.cfg[new] = val
        CTK.cfg.set_sub_node (new, tmp)

    vsrv_num = re.findall (r'^%s/(\d+)/vmatch'%(URL_APPLY), CTK.request.url)[0]
    pre      = 'vserver!%s!match' %(vsrv_num)

    op = CTK.post.get_val ('op')
    if op == 'or':
        # Ensure the first match is set
        # before adding a second entry.
        if not CTK.cfg.get_val (pre):
            return CTK.cfg_reply_ajax_ok()

        move (pre, "%s!left"%(pre))
        CTK.cfg[pre] = 'v_or'
        return CTK.cfg_reply_ajax_ok()

    elif op == 'del_1':
        # Is the second rule empty?
        if not CTK.cfg.get_val ('%s!right'%(pre)):
            del (CTK.cfg[pre])
            return CTK.cfg_reply_ajax_ok()

        move ('%s!right'%(pre), pre)
        return CTK.cfg_reply_ajax_ok()

    elif op == 'del_2':
        # Is the first rule empty?
        if not CTK.cfg.get_val ('%s!left'%(pre)):
            del (CTK.cfg[pre])
            return CTK.cfg_reply_ajax_ok()

        move ('%s!left'%(pre), pre)
        return CTK.cfg_reply_ajax_ok()

    return {'ret': 'fail'}


class RuleLink (CTK.Link):
    def __init__ (self, vsrv_num, rule_num, content):
        CTK.Link.__init__ (self, '/vserver/%s/rule/%s'%(vsrv_num, rule_num), content)
        self.vsrv_num = vsrv_num
        self.rule_num = rule_num

    def Render (self):
        from SelectionPanel import COOKIE_NAME_DEFAULT

        props = {'id':          self.id,
                 'vsrv_num':    self.vsrv_num,
                 'rule_num':    self.rule_num,
                 'cookie_name': COOKIE_NAME_DEFAULT}

        render = CTK.Link.Render (self)
        render.js += """$('#%(id)s').click(function(){
            $.cookie ('%(cookie_name)s', '%(rule_num)s_%(vsrv_num)s', { path: '/vserver/%(vsrv_num)s/rule'});
            window.location.replace ('/vserver/%(vsrv_num)s/rule');
            return false;
        });"""%(props)

        return render


class BehaviorWidget (CTK.Container):
    def __init__ (self, vsrv_num):
        CTK.Container.__init__ (self)

        # List
        table = CTK.Table({'id': 'rules-table'})
        table.set_header(1)
        table += [CTK.RawHTML('<span title="%s">%s</span>'%(_(x[0]),_(x[1]))) for x in BEHAVIOR_TAGS]

        rules = CTK.cfg.keys('vserver!%s!rule'%(vsrv_num))
        rules.sort (lambda x,y: cmp(int(x), int(y)))
        rules.reverse()

        for r in rules:
            table += self._get_row (vsrv_num, r)

        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Behavior Rules')))
        self += CTK.RawHTML (js=JS_TR_ODD+JS_TR_INACTIVE)
        self += CTK.Indenter (table)

        if rules:
            first_rule = rules[0]
        else:
            first_rule = '0'

        button = CTK.Button(_('Rule Management'), {'id':'rule-management'})
        button.bind ('click', "window.location = '/vserver/%s/rule/%s'"%(vsrv_num, first_rule))
        self += CTK.Indenter (button)


    def _get_row (self, vsrv_num, r):
        rule = Rule ('vserver!%s!rule!%s!match'%(vsrv_num, r))
        rule_name = rule.GetName()
        link = RuleLink (vsrv_num, r, CTK.RawHTML ('<span title="%s">%s</span>' %(rule_name, rule_name)))

        handler, auth, root, secure, enc, exp, flcache, timeout, shaping, log, final, active = [None for x in range(12)]

        tmp = CTK.cfg.get_val ('vserver!%s!rule!%s!handler'%(vsrv_num, r), '')
        if tmp:
            handler = CTK.RawHTML (_(filter (lambda x: x[0] == tmp, HANDLERS)[0][1]))

        tmp = CTK.cfg.get_val ('vserver!%s!rule!%s!auth'%(vsrv_num, r), '')
        if tmp:
            auth = CTK.RawHTML (_(filter (lambda x: x[0] == tmp, VALIDATORS)[0][1]))

        tmp = (CTK.cfg.get_val ('vserver!%s!rule!%s!document_root'%(vsrv_num, r)))
        if tmp:
            root = CTK.ImageStock('tick', {'alt': tmp, 'title':tmp})

        tmp = bool (int (CTK.cfg.get_val ('vserver!%s!rule!%s!only_secure'%(vsrv_num, r), "0")))
        if tmp:
            tmp = _('Only accept Secure Connections')
            secure = CTK.ImageStock('tick', {'alt': tmp, 'title': tmp})

        tmp =  CTK.cfg.keys ('vserver!%s!rule!%s!encoder'%(vsrv_num, r))
        if tmp:
            diz = [_('Encoding enabled')]
            for e in tmp:
                val = CTK.cfg.get_val ('vserver!%s!rule!%s!encoder!%s'%(vsrv_num, r, e))
                if val == 'allow':
                    diz.append (e)
                elif val == 'forbid':
                    diz.append ("%s %s"%(_('no'), e))
            txt = ', '.join(diz)
            enc = CTK.ImageStock('tick', {'alt': txt, 'title':txt})

        tmp = CTK.cfg.get_val ('vserver!%s!rule!%s!expiration' %(vsrv_num, r))
        if tmp:
            tmp = tmp.capitalize()
            exp = CTK.ImageStock('tick', {'alt': tmp, 'title': tmp})

        tmp = CTK.cfg.get_val ('vserver!%s!rule!%s!timeout' %(vsrv_num, r))
        if tmp:
            tmp += _(' seconds')
            timeout = CTK.ImageStock('tick', {'alt': tmp, 'title': tmp})

        tmp = CTK.cfg.get_val ('vserver!%s!rule!%s!rate' %(vsrv_num, r))
        if tmp:
            tmp += _(' bytes per second')
            shaping = CTK.ImageStock('tick', {'alt': tmp, 'title': tmp})

        tmp = not bool (int (CTK.cfg.get_val ('vserver!%s!rule!%s!no_log' %(vsrv_num, r), "0")))
        if tmp:
            tmp = _('Logging enabled for this rule')
            log = CTK.ImageStock('tick', {'alt': tmp, 'title': tmp})

        tmp = bool (int (CTK.cfg.get_val('vserver!%s!rule!%s!match!final'%(vsrv_num,r), "1")))
        if tmp:
            tmp   = _('Prevents further rule evaluation')
            final = CTK.ImageStock('tick', {'alt': tmp, 'title': tmp})

        tmp = CTK.cfg.get_val('vserver!%s!rule!%s!flcache'%(vsrv_num,r))
        if tmp in ('allow', '1'):
            tmp     = _('Content can be cached')
            flcache = CTK.ImageStock('tick', {'alt': tmp, 'title': tmp})
        elif tmp in ('forbid', '0'):
            tmp     = _('Forbids content caching')
            flcache = CTK.ImageStock ('forbid', {'alt': tmp, 'title': tmp})

        return [link, handler, auth, root, secure, enc, flcache, exp, timeout, shaping, log, final]


class BasicsWidget (CTK.Container):
    def __init__ (self, vsrv_num):
        CTK.Container.__init__ (self)

        pre        = "vserver!%s" %(vsrv_num)
        url_apply  = "%s/%s" %(URL_APPLY, vsrv_num)
        is_default = CTK.cfg.get_lowest_entry("vserver") == int(vsrv_num)

        # Server ID
        if not is_default:
            table = CTK.PropsAuto (url_apply)
            table.Add (_('Virtual Server nickname'), CTK.TextCfg('%s!nick'%(pre)), _(NOTE_NICKNAME))

            self += CTK.RawHTML ('<h2>%s</h2>' %(_('Server ID')))
            self += CTK.Indenter (table)

        # Paths
        table = CTK.PropsAuto (url_apply)
        table.Add (_('Document Root'), CTK.TextCfg('%s!document_root'%(pre)),   _(NOTE_DOCUMENT_ROOT))
        table.Add (_('Directory Indexes'), CTK.TextCfg('%s!directory_index'%(pre)), _(NOTE_DIRECTORY_INDEX))

        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Paths')))
        self += CTK.Indenter (table)

        if self.is_index_used (vsrv_num):
            tip = _(NOTE_INDEX_USAGE)
        else:
            tip = '%s %s' %(_(NOTE_INDEX_USAGE), _('Neither are in use at the moment.'))
        self += CTK.Indenter (CTK.Notice ('information', CTK.RawHTML(tip)))

        # Network
        table = CTK.PropsAuto (url_apply)
        table.Add (_('Keep-alive'),      CTK.CheckCfgText('%s!keepalive'%(pre), True, _('Allow')), _(NOTE_KEEPALIVE))
        table.Add (_('Max Upload Size'), CTK.TextCfg('%s!post_max_len'%(pre), True), _(NOTE_MAX_UPLOAD_SIZE))

        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Network')))
        self += CTK.Indenter (table)

        # Advanced Virtual Hosting
        table = CTK.PropsAuto (url_apply)
        modul = CTK.PluginSelector('%s!evhost'%(pre), trans_options(Cherokee.support.filter_available(EVHOSTS)))
        table.Add (_('Method'), modul.selector_widget, _(NOTE_EVHOST), False)

        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Advanced Virtual Hosting')))
        self += CTK.Indenter (table)
        self += CTK.Indenter (modul)


    def is_index_used (self, vsrv_num):
        key     = 'vserver!%s!rule' %(vsrv_num)
        matches = ['common', 'exists']

        def recursive_find (key):
            if CTK.cfg.get_val(key) in matches:
                return True

            for k in CTK.cfg.keys(key):
                if recursive_find ('%s!%s' %(key,k)):
                    return True
            return False

        if recursive_find (key):
            return True
        return False


class ErrorHandlerWidget (CTK.Container):
    def __init__ (self, vsrv_num):
        CTK.Container.__init__ (self)

        pre        = "vserver!%s" %(vsrv_num)
        url_apply  = "%s/%s" %(URL_APPLY, vsrv_num)

        table = CTK.PropsAuto (url_apply)
        modul = CTK.PluginSelector ('%s!error_handler'%(pre), trans_options(Cherokee.support.filter_available(ERROR_HANDLERS)), vsrv_num=vsrv_num)
        table.Add (_('Method'), modul.selector_widget, _(NOTE_ERROR_HANDLER), False)

        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Error Handling hook')))
        self += CTK.Indenter (table)
        self += modul


class LogginWidgetContent (CTK.Container):
    def __init__ (self, vsrv_num, refreshable):
        CTK.Container.__init__ (self)

        pre        = "vserver!%s" %(vsrv_num)
        url_apply  = "%s/%s/log" %(URL_APPLY, vsrv_num)
        writers    = [('', _('Disabled'))] + trans_options(LOGGER_WRITERS)

        # Error writer
        table = CTK.PropsTable()
        table.Add (_('Write errors to'), CTK.ComboCfg('%s!error_writer!type'%(pre), writers), _(NOTE_ERRORS))

        writer = CTK.cfg.get_val ('%s!error_writer!type'%(pre))
        if writer == 'file':
            table.Add (_('Filename'), CTK.TextCfg('%s!error_writer!filename'%(pre)), _(NOTE_WRT_FILE))
        elif writer == 'exec':
            table.Add (_('Command'), CTK.TextCfg('%s!error_writer!command'%(pre)), _(NOTE_WRT_EXEC))

        submit = CTK.Submitter(url_apply)
        submit.bind ('submit_success', refreshable.JS_to_refresh())
        submit += table

        self += CTK.RawHTML ('<h2>%s</h2>' % (_('Error Logging')))
        self += CTK.Indenter (submit)

        # Access logger
        pre = 'vserver!%s!logger' %(vsrv_num)

        submit = CTK.Submitter(url_apply)
        submit.bind ('submit_success', refreshable.JS_to_refresh())
        submit += CTK.ComboCfg(pre, trans_options(Cherokee.support.filter_available(LOGGERS)))

        table = CTK.PropsTable()
        table.Add (_('Format'), submit, _(NOTE_LOGGERS))

        format = CTK.cfg.get_val(pre)
        if format:
            submit = CTK.Submitter(url_apply)
            submit.bind ('submit_success', refreshable.JS_to_refresh())
            submit += CTK.ComboCfg('%s!access!type'%(pre), trans_options(LOGGER_WRITERS))
            table.Add (_('Write accesses to'), submit, _(NOTE_ACCESSES))

            submit = CTK.Submitter(url_apply)
            writer = CTK.cfg.get_val ('%s!access!type'%(pre))
            if not writer:
                CTK.cfg['%s!access!type'%(pre)] = 'file'

            if writer == 'file' or not writer:
                # Set a default value
                if not CTK.cfg.get_val ('%s!access!filename'%(pre)):
                    CTK.cfg['%s!access!filename'%(pre)] = os.path.join (CHEROKEE_VAR_LOG, 'cherokee.log')

                submit += CTK.TextCfg('%s!access!filename'%(pre), False)
                table.Add (_('Filename'), submit, _(NOTE_WRT_FILE))
            elif writer == 'exec':
                submit += CTK.TextCfg('%s!access!command'%(pre))
                table.Add (_('Command'), submit, _(NOTE_WRT_EXEC))

            if format == 'custom':
                table_macros = CTK.Table ({'class': 'custom-logger-macros'})
                for macro, desc in CUSTOM_MACROS_LIST:
                    table_macros += [CTK.RawHTML('${%s}'%(macro)), CTK.RawHTML (_(desc))]

                collaps  = CTK.CollapsibleEasy ((_('Show macros'), _('Hide')))
                collaps += table_macros

                macros = CTK.Container()
                macros += CTK.RawHTML (_(NOTE_LOGGER_TEMPLATE))
                macros += collaps

                submit = CTK.Submitter(url_apply)
                submit += CTK.TextCfg('%s!access_template'%(pre))
                table.Add (_('Template: '), submit, macros)

        self += CTK.RawHTML ('<h2>%s</h2>' % (_('Access Logging')))
        self += CTK.Indenter (table)

        # Properties
        if CTK.cfg.get_val (pre):
            table = CTK.PropsTable()
            table.Add (_('Time standard'), CTK.ComboCfg ('%s!utc_time'%(pre), trans_options(UTC_TIME)), _(NOTE_UTC_TIME))
            table.Add (_('Accept Forwarded IPs'), CTK.CheckCfgText ('%s!x_real_ip_enabled'%(pre), False, _('Accept')), _(NOTE_X_REAL_IP))

            if int (CTK.cfg.get_val('%s!x_real_ip_enabled'%(pre), "0")):
                table.Add (_('Don\'t check origin'), CTK.CheckCfgText ('%s!x_real_ip_access_all'%(pre), False, _('Do not check')), _(NOTE_X_REAL_IP_ALL))

                if not int (CTK.cfg.get_val ('%s!x_real_ip_access_all'%(pre), "0")):
                    table.Add (_('Accept from Hosts'), CTK.TextCfg ('%s!x_real_ip_access'%(pre)), _(NOTE_X_REAL_IP_ACCESS))

            submit = CTK.Submitter(url_apply)
            submit.bind ('submit_success', refreshable.JS_to_refresh())
            submit += table

            self += CTK.RawHTML ('<h3>%s</h3>' % (_('Logging Options')))
            self += CTK.Indenter (submit)

def LogginWidgetContent_Apply():
    # Apply
    ret = CTK.cfg_apply_post()

    # Check Error Writer stale-entries
    vsrv_num = re.findall (r'^%s/(\d+)/log'%(URL_APPLY), CTK.request.url)[0]
    pre      = 'vserver!%s!error_writer' %(vsrv_num)

    if not CTK.cfg.get_val ('%s!type'%(pre)):
        del (CTK.cfg[pre])

    return ret


class LogginWidget (CTK.Container):
    def __init__ (self, vsrv_num):
        CTK.Container.__init__ (self)

        # Content
        refresh = CTK.Refreshable({'id': 'vserver_login'})
        refresh.register (lambda: LogginWidgetContent(vsrv_num, refresh).Render())
        self += refresh


class SecutiryWidgetContent (CTK.Container):
    def __init__ (self, vsrv_num, refreshable):
        CTK.Container.__init__ (self)

        pre        = "vserver!%s" %(vsrv_num)
        url_apply  = "%s/%s" %(URL_APPLY, vsrv_num)

        # Required SSL/TLS values
        table = CTK.PropsTable()
        table.Add (_('Certificate'),     CTK.TextCfg ('%s!ssl_certificate_file'%(pre), True), _(NOTE_CERT))
        table.Add (_('Certificate key'), CTK.TextCfg ('%s!ssl_certificate_key_file'%(pre), True), _(NOTE_CERT_KEY))

        submit = CTK.Submitter (url_apply)
        submit += table

        self += CTK.RawHTML ('<h2>%s</h2>' % (_('Required SSL/TLS Values')))
        self += CTK.Indenter (submit)

        # Advanced options
        table = CTK.PropsTable()
        table.Add (_('Ciphers'),               CTK.TextCfg ('%s!ssl_ciphers' %(pre), True), _(NOTE_CIPHERS))
        table.Add (_('Server Preference'),     CTK.CheckCfgText ('%s!ssl_cipher_server_preference' % (pre), True, _('Prefer')), _(NOTE_CIPHER_SERVER_PREFERENCE))
        table.Add (_('Client Certs. Request'), CTK.ComboCfg('%s!ssl_client_certs' %(pre), trans_options(CLIENT_CERTS)), _(NOTE_CLIENT_CERTS))
        table.Add (_('Compression'),           CTK.CheckCfgText ('%s!ssl_compression' % (pre), False, _('Enable')), _(NOTE_COMPRESSION))
        table.Add (_('DH length'),             CTK.ComboCfg('%s!ssl_dh_length' %(pre), [('','Auto'), ('512', '512 bits'), ('1024', '1024 bits'), ('2048', '2048 bits'), ('4096', '4096 bits')]), _(NOTE_DH_LENGTH))

        if CTK.cfg.get_val('%s!ssl_client_certs' %(pre)):
            table.Add (_('CA List'), CTK.TextCfg ('%s!ssl_ca_list_file' %(pre), False), _(NOTE_CA_LIST))

            if CTK.cfg.get_val('%s!ssl_ca_list_file' %(pre)):
                table.Add (_('Verify Depth'), CTK.TextCfg ('%s!ssl_verify_depth' %(pre), False, {'size': 4}), _(NOTE_VERIFY_DEPTH))

        submit = CTK.Submitter (url_apply)
        submit.bind ('submit_success', refreshable.JS_to_refresh())
        submit += table

        self += CTK.RawHTML ('<h2>%s</h2>' % (_('Advanced Options')))
        self += CTK.Indenter (submit)

        # HSTS
        table = CTK.PropsTable()
        table.Add (_('Enable HSTS'), CTK.CheckCfgText ('%s!hsts'%(pre), False, _('Accept')), _(NOTE_HSTS))

        if int(CTK.cfg.get_val('%s!hsts' %(pre), "0")):
            table.Add (_('HSTS Max-Age'),       CTK.TextCfg ('%s!hsts!max_age'%(pre), True, {'optional_string':_("One year")}), _(NOTE_HSTS_MAXAGE))
            table.Add (_('Include Subdomains'), CTK.CheckCfgText ('%s!hsts!subdomains'%(pre), True, _('Include all')), _(NOTE_HSTS_SUBDOMAINS))

        submit = CTK.Submitter (url_apply)
        submit.bind ('submit_success', refreshable.JS_to_refresh())
        submit += table

        self += CTK.RawHTML ('<h2>%s</h2>' % (_('HTTP Strict Transport Security (HSTS)')))
        self += CTK.Indenter (submit)


class SecurityWidget (CTK.Container):
    def __init__ (self, vsrv_num):
        CTK.Container.__init__ (self)

        # Content
        refresh = CTK.Refreshable ({'id': 'vserver_security'})
        refresh.register (lambda: SecutiryWidgetContent(vsrv_num, refresh).Render())
        self += refresh


class RenderContent (CTK.Container):
    def __init__ (self, vsrv_num, vsrv_nam=''):
        CTK.Container.__init__ (self)

        tabs = CTK.Tab()
        tabs.Add (_('Basics'),        BasicsWidget (vsrv_num))
        tabs.Add (_('Host Match'),    HostMatchWidget (vsrv_num))
        tabs.Add (_('Behavior'),      BehaviorWidget (vsrv_num))
        tabs.Add (_('Error Handler'), ErrorHandlerWidget (vsrv_num))
        tabs.Add (_('Logging'),       LogginWidget (vsrv_num))
        tabs.Add (_('Security'),      SecurityWidget (vsrv_num))

        self += CTK.RawHTML ('<h2>%s: %s</h2>' %(_('Virtual Server'), CTK.escape_html (vsrv_nam)))
        self += tabs


class Render:
    def __call__ (self):
        # Parse request
        vsrv_num = re.findall (r'^%s/([\d]+)'%(URL_BASE), CTK.request.url)[0]
        vsrv_nam = CTK.cfg.get_val ("vserver!%s!nick" %(vsrv_num), _("Unknown"))

        # Ensure the vserver exists
        if not CTK.cfg.keys ("vserver!%s"%(vsrv_num)):
            return CTK.HTTP_Redir("/vserver")

        # Content
        cont = RenderContent (vsrv_num, vsrv_nam)
        return cont.Render().toJSON()


CTK.publish (r'^%s/[\d]+$'        %(URL_BASE),  Render)
CTK.publish (r'^%s/[\d]+'         %(URL_APPLY), CTK.cfg_apply_post,        method="POST", validation=VALIDATIONS)
CTK.publish (r'^%s/[\d]+/log'     %(URL_APPLY), LogginWidgetContent_Apply, method="POST", validation=VALIDATIONS)
CTK.publish (r'^%s/[\d]+/vmatch'  %(URL_APPLY), HostMatchWidget_Apply,     method="POST")
CTK.publish (r'^%s/[\d]+/clone$'  %(URL_BASE),  commit_clone)

