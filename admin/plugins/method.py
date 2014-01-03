# Cheroke Admin: HTTP Method rule plug-in
#
# Authors:
#      Taher Shihadeh <taher@unixwars.com>
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

from Rule import RulePlugin
from util import *

URL_APPLY = '/plugin/method/apply'

METHODS = [
    ('', _('Choose')),
    ('get',              'GET'),
    ('post',             'POST'),
    ('head',             'HEAD'),
    ('put',              'PUT'),
    ('options',          'OPTIONS'),
    ('delete',           'DELETE'),
    ('trace',            'TRACE'),
    ('connect',          'CONNECT'),
    ('purge',            'PURGE'),
    ('copy',             'COPY'),
    ('lock',             'LOCK'),
    ('mkcol',            'MKCOL'),
    ('move',             'MOVE'),
    ('notify',           'NOTIFY'),
    ('poll',             'POLL'),
    ('propfind',         'PROPFIND'),
    ('proppatch',        'PROPPATCH'),
    ('search',           'SEARCH'),
    ('subscribe',        'SUBSCRIBE'),
    ('unlock',           'UNLOCK'),
    ('unsubscribe',      'UNSUBSCRIBE'),
    ('report',           'REPORT'),
    ('patch',            'PATCH'),
    ('version-control',  'VERSION-CONTROL'),
    ('checkout',         'CHECKOUT'),
    ('uncheckout',       'UNCHECKOUT'),
    ('checkin',          'CHECKIN'),
    ('update',           'UPDATE'),
    ('label',            'LABEL'),
    ('mkworkspace',      'MKWORKSPACE'),
    ('mkactivity',       'MKACTIVITY'),
    ('baseline-control', 'BASELINE-CONTROL'),
    ('merge',            'MERGE')
]

NOTE_METHOD  = N_("The HTTP method that should match this rule.")

def commit():
    # POST info
    key        = CTK.post.pop ('key', None)
    vsrv_num   = CTK.post.pop ('vsrv_num', None)
    new_method = CTK.post.pop ('tmp!method', None)

    # New entry
    if new_method:
        next_rule, next_pre = cfg_vsrv_rule_get_next ('vserver!%s'%(vsrv_num))

        CTK.cfg['%s!match'%(next_pre)]        = 'method'
        CTK.cfg['%s!match!method'%(next_pre)] = new_method

        return {'ret': 'ok', 'redirect': '/vserver/%s/rule/%s' %(vsrv_num, next_rule)}

    # Modifications
    return CTK.cfg_apply_post()


class Plugin_method (RulePlugin):
    def __init__ (self, key, **kwargs):
        RulePlugin.__init__ (self, key)
        is_new = key.startswith('tmp')
        idx    = (1,0)[is_new]

        # Set the first method is empty
        if not CTK.cfg.get_val ('%s!method'%(key)):
            CTK.cfg ['%s!method'%(key)] = METHODS[idx:][0][0]

        table = CTK.PropsTable()
        table.Add (_('Method'), CTK.ComboCfg('%s!method'%(key), METHODS[idx:], ({}, {'class': 'noauto'})[is_new]), _(NOTE_METHOD))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('key', key)
        submit += CTK.Hidden ('vsrv_num', kwargs.pop('vsrv_num', ''))
        submit += table
        self += submit

    def GetName (self):
        method = CTK.cfg.get_val ('%s!method' %(self.key), '')
        return "%s %s" % (_('Method'), method.upper())


CTK.publish ("^%s"%(URL_APPLY), commit, method="POST")
