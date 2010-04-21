# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2010 Alvaro Lopez Ortega
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

import re

from Rule import Rule
from consts import *
from configured import *

URL_BASE  = r'^/vserver/([\d]+)/rule/content/([\d]+)/?$'
URL_APPLY = r'^/vserver/([\d]+)/rule/content/([\d]+)/apply$'
URL_CLONE = r'^/vserver/([\d]+)/rule/content/([\d]+)/clone$'

NOTE_TIMEOUT         = N_('Apply a custom timeout to the connections matching this rule.')
NOTE_HANDLER         = N_('How the connection will be handled.')
NOTE_HTTPS_ONLY      = N_('Enable to allow access to the resource only by https.')
NOTE_ALLOW_FROM      = N_('List of IPs and subnets allowed to access the resource.')
NOTE_VALIDATOR       = N_('Which, if any, will be the authentication method.')
NOTE_EXPIRATION      = N_('Points how long the files should be cached')
NOTE_RATE            = N_("Set an outbound traffic limit. It must be specified in Bytes per second.")
NOTE_NO_LOG          = N_("Do not log requests matching this rule.")
NOTE_EXPIRATION_TIME = N_("""How long from the object can be cached.<br />
The <b>m</b>, <b>h</b>, <b>d</b> and <b>w</b> suffixes are allowed for minutes, hours, days, and weeks. Eg: 2d.
""")

VALIDATIONS = [
    ("vserver![\d]+!rule![\d]+!document_root",   validations.is_dev_null_or_local_dir_exists),
    ("vserver![\d]+!rule![\d]+!allow_from",      validations.is_ip_or_netmask_list),
    ("vserver![\d]+!rule![\d]+!rate",            validations.is_number_gt_0),
    ("vserver![\d]+!rule![\d]+!timeout",         validations.is_number_gt_0),
    ("vserver![\d]+!rule![\d]+!expiration!time", validations.is_time)
]

HELPS = [
    ('config_virtual_servers_rule', N_("Behavior rules")),
    ('modules_encoders_gzip',       N_("GZip encoder")),
    ('modules_encoders_deflate',    N_("Deflate encoder")),
    ('cookbook_authentication',     N_('Authentication')),
    ('modules_validators',          N_("Authentication modules"))
]

ENCODE_OPTIONS = [
    ('',       N_('Leave unset')),
    ('allow',  N_('Allow')),
    ('forbid', N_('Forbid'))
]


def Clone():
    vsrv, rule = re.findall(URL_CLONE, CTK.request.url)[0]
    next = CTK.cfg.get_next_entry_prefix ('vserver!%s!rule'%(vsrv))

    CTK.cfg.clone ('vserver!%s!rule!%s'%(vsrv,rule), next)
    return CTK.cfg_reply_ajax_ok()


class TrafficWidget (CTK.Container):
    def __init__ (self, vsrv, rule, apply):
        CTK.Container.__init__ (self)
        pre = 'vserver!%s!rule!%s' %(vsrv, rule)

        table = CTK.PropsTable()
        table.Add (_('Limit traffic to'), CTK.TextCfg ('%s!rate'%(pre), False), _(NOTE_RATE))
        submit = CTK.Submitter (apply)
        submit += table

        self += CTK.RawHTML ("<h2>%s</h2>" % (_('Traffic Shaping')))
        self += CTK.Indenter (submit)


class SecurityWidget (CTK.Container):
    def __init__ (self, vsrv, rule, apply):
        CTK.Container.__init__ (self)
        pre = 'vserver!%s!rule!%s' %(vsrv, rule)

        # Logging
        table = CTK.PropsTable()
        table.Add (_('Skip Logging'), CTK.CheckCfgText ('%s!no_log'%(pre), False), _(NOTE_NO_LOG))
        submit = CTK.Submitter (apply)
        submit += table

        self += CTK.RawHTML ("<h2>%s</h2>" % (_('Logging')))
        self += CTK.Indenter (submit)

        # Access Restrictions
        table = CTK.PropsTable()
        table.Add (_('Only https'), CTK.CheckCfgText ('%s!only_secure'%(pre), False), _(NOTE_HTTPS_ONLY))
        table.Add (_('Allow From'), CTK.TextCfg ('%s!allow_from'%(pre), True), _(NOTE_ALLOW_FROM))
        submit = CTK.Submitter (apply)
        submit += table

        self += CTK.RawHTML ("<h2>%s</h2>" % (_('Access Restrictions')))
        self += CTK.Indenter (submit)

        # Authentication
        modul = CTK.PluginSelector('%s!auth'%(pre), Cherokee.support.filter_available (VALIDATORS))

        table = CTK.PropsTable()
        table.Add (_('Validation Mechanism'), modul.selector_widget, _(NOTE_VALIDATOR))

        self += CTK.RawHTML ("<h2>%s</h2>" % (_('Authentication')))
        self += CTK.Indenter (table)
        self += modul


class TimeWidget (CTK.Container):
    class Expiration (CTK.Container):
        def __init__ (self, pre, apply, refresh):
            CTK.Container.__init__ (self)

            table = CTK.PropsTable()
            table.Add (_('Expiration'), CTK.ComboCfg ('%s!expiration'%(pre), EXPIRATION_TYPE), _(NOTE_EXPIRATION))

            if CTK.cfg.get_val ('%s!expiration'%(pre)) == 'time':
                table.Add (_('Time to expire'), CTK.TextCfg ('%s!expiration!time'%(pre), False), _(NOTE_EXPIRATION_TIME))

            submit = CTK.Submitter (apply)
            submit.bind ('submit_success', refresh.JS_to_refresh())
            submit += table
            self += submit

    def __init__ (self, vsrv, rule, apply):
        CTK.Container.__init__ (self)
        pre = 'vserver!%s!rule!%s' %(vsrv, rule)

        # Timeout
        table = CTK.PropsTable()
        table.Add (_('Timeout'), CTK.TextCfg ('%s!timeout'%(pre), True), _(NOTE_TIMEOUT))
        submit = CTK.Submitter (apply)
        submit += table

        self += CTK.RawHTML ("<h2>%s</h2>" % (_('Connections Timeout')))
        self += CTK.Indenter (submit)

        # Expiration
        refresh = CTK.Refreshable({'id': 'time_expiration'})
        refresh.register (lambda: TimeWidget.Expiration(pre, apply, refresh).Render())

        self += CTK.RawHTML ("<h2>%s</h2>" % (_('Content Expiration')))
        self += CTK.Indenter (refresh)


class EncodingWidget (CTK.Container):
    def __init__ (self, vsrv, rule, apply):
        CTK.Container.__init__ (self)
        pre = 'vserver!%s!rule!%s!encoder' %(vsrv, rule)
        encoders = Cherokee.support.filter_available (ENCODERS)

        table = CTK.PropsTable()
        for e,e_name in encoders:
            note  = _("Use the %s encoder whenever the client requests it.") %(_(e_name))
            table.Add (_("%s Support") %(_(e_name)), CTK.ComboCfg('%s!%s'%(pre,e), ENCODE_OPTIONS), note)

        submit = CTK.Submitter (apply)
        submit += table

        self += CTK.RawHTML ("<h2>%s</h2>" % (_('Information Encoders')))
        self += CTK.Indenter (submit)


class RuleWidget (CTK.Refreshable):
    class Content (CTK.Container):
        def __init__ (self, refresh, refresh_header, vsrv, rule, apply):
            CTK.Container.__init__ (self)
            pre = 'vserver!%s!rule!%s!match' %(vsrv, rule)

            rule = Rule (pre)
            rule.bind ('changed',        refresh.JS_to_refresh() + refresh_header.JS_to_refresh())
            rule.bind ('submit_success', refresh.JS_to_refresh() + refresh_header.JS_to_refresh())

            self += CTK.RawHTML ("<h2>%s</h2>" % (_('Matching Rule')))
            self += CTK.Indenter (rule)

    def __init__ (self, vsrv, rule, apply, refresh_header):
        CTK.Refreshable.__init__ (self, {'id': 'rule'})
        self.register (lambda: self.Content(self, refresh_header, vsrv, rule, apply).Render())


class Header (CTK.Container):
    def __init__ (self, refresh, vsrv_num, rule_num, vsrv_nam):
        CTK.Container.__init__(self)

        rule = Rule ('vserver!%s!rule!%s!match'%(vsrv_num, rule_num))
        rule_nam = rule.GetName()
        self += CTK.RawHTML ('<h2><a href="/vserver/%s">%s</a> &rarr; %s</h2>' %(vsrv_num, vsrv_nam, rule_nam))


class HandlerWidget (CTK.Container):
    def __init__ (self, vsrv, rule, apply):
        CTK.Container.__init__ (self)
        pre = 'vserver!%s!rule!%s!handler' %(vsrv, rule)

        modul = CTK.PluginSelector(pre, Cherokee.support.filter_available (HANDLERS))

        table = CTK.PropsTable()
        table.Add (_('Handler'), modul.selector_widget, _(NOTE_HANDLER))

        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Handler')))
        self += CTK.Indenter (table)
        self += modul


class Render:
    def __call__ (self):
        # Parse request
        vsrv_num, rule_num = re.findall (URL_BASE, CTK.request.url)[0]
        vsrv_nam = CTK.cfg.get_val ("vserver!%s!nick" %(vsrv_num), _("Unknown"))

        url_apply = '/vserver/%s/rule/content/%s/apply' %(vsrv_num, rule_num)

        # Ensure the rule exists
        if not CTK.cfg.keys('vserver!%s!rule!%s'%(vsrv_num, rule_num)):
            return CTK.HTTP_Redir ('/vserver/%s' %(vsrv_num))

        # Header refresh
        refresh = CTK.Refreshable ({'id': 'entry_header'})
        refresh.register (lambda: Header(refresh, vsrv_num, rule_num, vsrv_nam).Render())

        # Tabs
        tabs = CTK.Tab()
        tabs.Add (_('Rule'),            RuleWidget (vsrv_num, rule_num, url_apply, refresh))
        tabs.Add (_('Handler'),         HandlerWidget (vsrv_num, rule_num, url_apply))
        tabs.Add (_('Encoding'),        EncodingWidget (vsrv_num, rule_num, url_apply))
        tabs.Add (_('Time'),            TimeWidget (vsrv_num, rule_num, url_apply))
        tabs.Add (_('Security'),        SecurityWidget (vsrv_num, rule_num, url_apply))
        tabs.Add (_('Traffic Shaping'), TrafficWidget (vsrv_num, rule_num, url_apply))

        cont = CTK.Container()
        cont += refresh
        cont += tabs

        return cont.Render().toJSON()



CTK.publish (URL_BASE, Render)
CTK.publish (URL_CLONE, Clone, method="POST")
CTK.publish (URL_APPLY, CTK.cfg_apply_post, validation=VALIDATIONS, method="POST")
