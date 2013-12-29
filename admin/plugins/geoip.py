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
from Flags import ComboFlags, CheckListFlags
from util import *

URL_APPLY = '/plugin/geoip/apply'

ISO3166_URL      = "http://www.iso.org/iso/country_codes/iso_3166_code_lists/english_country_names_and_code_elements.htm"
NOTE_NEW_COUNTRY = N_("Add the initial country. It's possible to add more later on.")
NOTE_ADD_COUNTRY = N_("Pick an additional country to add to the country list.")
NOTE_COUNTRIES   = N_("""List of countries from the client IPs. It must use the
<a target=\"_blank\" href=\"%s\">ISO 3166</a> country notation.""") % (ISO3166_URL)

def commit():
    # POST info
    key       = CTK.post.pop ('key', None)
    vsrv_num  = CTK.post.pop ('vsrv_num', None)

    # New
    post_keys = CTK.post.keys()
    selected  = []
    for p in post_keys:
        if p.startswith('tmp!countries!'):
            if int(CTK.post[p]):
                selected.append (p[-2:])

    if selected:
        next_rule, next_pre = cfg_vsrv_rule_get_next ('vserver!%s'%(vsrv_num))
        CTK.cfg['%s!match'%(next_pre)]           = 'geoip'
        CTK.cfg['%s!match!countries'%(next_pre)] = ','.join(selected)
        return {'ret': 'ok', 'redirect': '/vserver/%s/rule/%s' %(vsrv_num, next_rule)}

    # Modifications
    for p in post_keys:
        if p.startswith(key):
            value = CTK.post[p]
            if value and int(value):
                selected.append (p[-2:])

    CTK.cfg[key]                  = 'geoip'
    CTK.cfg["%s!countries"%(key)] = ','.join(selected)
    return CTK.cfg_reply_ajax_ok()


CHECKBOX_JS = """
$('.flag_checkbox').attr('checked', %s);
"""
class Plugin_geoip (RulePlugin):
    def __init__ (self, key, **kwargs):
        RulePlugin.__init__ (self, key)
        self.vsrv_num = kwargs.pop('vsrv_num', '')
        props = ({},{'class':'noauto'})[key.startswith('tmp')]

        submit = CTK.Submitter (URL_APPLY)
        select_all  = CTK.Button (_('Select all'))
        select_none = CTK.Button (_('Select none'))
        select_all.bind  ('click', CHECKBOX_JS % 'true'  + submit.JS_to_submit())
        select_none.bind ('click', CHECKBOX_JS % 'false' + submit.JS_to_submit())

        # GUI
        box  = CTK.Box({'class': 'flags-buttons'})
        box += select_all
        box += select_none
        submit += box
        submit += CheckListFlags ('%s!countries'%(self.key), props)
        submit += CTK.Hidden ('key', self.key)
        submit += CTK.Hidden ('vsrv_num', self.vsrv_num)

        self += submit

    def GetName (self):
        match = CTK.cfg.get_val ('%s!countries'%(self.key), '')
        return "From countries %s" %(match)

# Validation, and Public URLs
CTK.publish ("^%s"%(URL_APPLY), commit, method="POST")
