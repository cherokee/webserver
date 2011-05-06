# -*- coding: utf-8 -*-
#
# Cherokee-admin's Common Static wizard
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2010 Alvaro Lopez Ortega
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

import re
import CTK
import Wizard
from util import *
from configured import *

NOTE_WELCOME_H1 = N_("Welcome to the Cache wizard")
NOTE_WELCOME_P1 = N_("This wizard adds a rule to configure the cache mechanism shipped with Cherokee.")
NOTE_WELCOME_P2 = N_("It will boost the performance of your virtual server by caching content, and thus optimizing subsequent requests of the same Web resource.")

NOTE_POLICY   = N_("What do you want the server to cache?")
NOTE_ENC_EXTS = N_("Which extensions you want the Cache to store?")

PREFIX    = 'tmp!wizard!flcache'
URL_APPLY = r'/wizard/vserver/flcache/apply'

POLICIES = [
    ('dynamic', N_('Cacheable Dynamic Responses')),
    ('encoded', N_('Encoded responses of static files'))
]

ENCODED_EXTS_DEFAULT = "js,css,html,htm,xml"
ENCODED_EXTS_CONF    = """
%(rule_pre)s!match!final = 0
%(rule_pre)s!match = and
%(rule_pre)s!match!left = extensions
%(rule_pre)s!match!left!check_local_file = 0
%(rule_pre)s!match!left!extensions = %(extensions)s
%(rule_pre)s!match!right = header
%(rule_pre)s!match!right!complete = 0
%(rule_pre)s!match!right!header = Accept-Encoding
%(rule_pre)s!match!right!type = provided
%(rule_pre)s!flcache = allow
%(rule_pre)s!flcache!policy = all_but_forbidden
%(rule_pre)s!encoder!gzip = allow
"""

GLOBAL_CACHE_CONF    = """
%(rule_pre)s!match!final = 0
%(rule_pre)s!match = directory
%(rule_pre)s!match!directory = /
%(rule_pre)s!flcache = allow
%(rule_pre)s!flcache!policy = explicitly_allowed
"""

class Commit:
    def Commit_Rule (self):
        vsrv_num = CTK.cfg.get_val ('%s!vsrv_num'%(PREFIX))
        tipe     = CTK.cfg.get_val ('%s!policy'  %(PREFIX))
        vsrv_pre = 'vserver!%s' %(vsrv_num)

        # Next rule
        x, rule_pre = cfg_vsrv_rule_get_next (vsrv_pre)

        if tipe == 'encoded':
            # Encoded content
            extensions = CTK.cfg.get_val ('%s!encoded_exts'%(PREFIX))
            config = ENCODED_EXTS_CONF % (locals())
        else:
            # General caching
            config = GLOBAL_CACHE_CONF % (locals())

        # Apply the config
        CTK.cfg.apply_chunk (config)
        CTK.cfg.normalize ('%s!rule'%(vsrv_pre))

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()

    def __call__ (self):
        if CTK.post.pop('final'):
            CTK.cfg_apply_post()
            return self.Commit_Rule()

        return CTK.cfg_apply_post()


class Create:
    def __call__ (self):
        combo_type = CTK.ComboCfg ('%s!policy'%(PREFIX), trans_options(POLICIES))

        table = CTK.PropsTable()
        table.Add (_('Type of Caching'), combo_type, _(NOTE_POLICY))

        encoded_table = CTK.PropsTable()
        encoded_table.Add (_('Extensions'), CTK.TextCfg ('%s!encoded_exts'%(PREFIX), False, {'value': ENCODED_EXTS_DEFAULT}), _(NOTE_ENC_EXTS))
        encoded_box = CTK.Box({'style':'display:none;'})
        encoded_box += encoded_table

        combo_type.bind ('change',
                         "if ($(this).val() == 'dynamic') {%s} else {%s}"
                         %(encoded_box.JS_to_hide(), encoded_box.JS_to_show()))

        cont = CTK.Container()

        submit = CTK.Submitter (URL_APPLY)
        submit += table
        submit += encoded_box
        cont += submit

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')
        cont += submit

        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class Welcome:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('flcache', {'class': 'wizard-descr'})

        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P2)))
        cont += box

        # Send the VServer num
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
        cont += submit

        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


# Rule
CTK.publish ('^/wizard/vserver/(\d+)/flcache$',  Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/flcache/2', Create)
CTK.publish (r'^%s$'%(URL_APPLY), Commit, method="POST")
