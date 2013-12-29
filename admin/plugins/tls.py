# Cheroke Admin
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
from Rule import RulePlugin
from util import *

URL_APPLY = '/plugin/tls/apply'


def commit():
    # Rule number
    key      = CTK.post.pop ('key', None)
    vsrv_num = CTK.post.pop ('vsrv_num', None)

    # Add a new entry
    if key.startswith('tmp'):
        next_rule, next_pre = cfg_vsrv_rule_get_next ('vserver!%s'%(vsrv_num))
        CTK.cfg['%s!match'%(next_pre)] = 'tls'

        return {'ret': 'ok', 'redirect': '/vserver/%s/rule/%s' %(vsrv_num, next_rule)}

    return {'ret': 'ok'}


class Plugin_tls (RulePlugin):
    def __init__ (self, key, **kwargs):
        RulePlugin.__init__ (self, key)
        vsrv_num = kwargs.pop('vsrv_num', '')

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('key',      key)
        submit += CTK.Hidden ('vsrv_num', vsrv_num)
        self += submit

    def GetName (self):
        return _("Is SSL/TLS")


CTK.publish ("^%s"%(URL_APPLY), commit, method="POST")
