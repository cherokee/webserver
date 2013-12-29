# Cheroke Admin: RRD plug-in
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

URL_APPLY = '/plugin/%s/apply'

def apply (extension):
    # POST info
    key      = CTK.post.pop ('key', None)
    vsrv_num = CTK.post.pop ('vsrv_num', None)
    new_val  = CTK.post.pop ('tmp!%s'%(extension), None)

    # New
    if new_val:
        next_rule, next_pre = cfg_vsrv_rule_get_next ('vserver!%s'%(vsrv_num))

        CTK.cfg['%s!match'%(next_pre)]               = extension
        CTK.cfg['%s!match!%s'%(next_pre, extension)] = new_val

        return {'ret': 'ok', 'redirect': '/vserver/%s/rule/%s' %(vsrv_num, next_rule)}

    # Modifications
    return CTK.cfg_apply_post()


class PluginSimple (RulePlugin):
    def __init__ (self, key, title, extension, note, name_desc, validation_fn=None, **kwargs):
        RulePlugin.__init__ (self, key)

        self.extension = extension
        self.name_desc = name_desc

        url_apply = URL_APPLY %(extension)
        props     = ({},{'class': 'noauto'})[key.startswith('tmp')]

        table = CTK.PropsTable()
        table.Add (title, CTK.TextCfg('%s!%s'%(key, extension), False, props), note)

        submit = CTK.Submitter (url_apply)
        submit += CTK.Hidden ('key', key)
        submit += CTK.Hidden ('vsrv_num', kwargs.pop('vsrv_num', ''))
        submit += table
        self += submit

        # Build validation
        if validation_fn:
            validation = [('tmp!%s'%(extension),                            validation_fn),
                          ('vserver![\d]+!rule![\d]+!match!%s'%(extension), validation_fn)]
        else:
            validation = []

        # Public URL
        CTK.publish ('^%s'%(url_apply), apply, method="POST", validation=validation, extension=extension)

    def GetName (self):
        value = CTK.cfg.get_val ('%s!%s' %(self.key, self.extension), '')
        return "%s %s" %(self.name_desc, value)
