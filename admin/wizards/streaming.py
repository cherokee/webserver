# -*- coding: utf-8 -*-
#
# Cherokee-admin's Media Streaming Wizard
#
# Authors:
#      Taher Shihadeh <taher@unixwars.com>
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

#
# Tested:
# 2010/04/14: Cherokee 0.99.41
#

import re
import string
import CTK
import Wizard
import validations
from util import *
from configured import CHEROKEE_PLUGINDIR

NOTE_WELCOME_H1 = N_("Welcome to the Streaming Wizard")
NOTE_WELCOME_P1 = N_("This Wizard Adds a rule to stream media files.")

PREFIX    = 'tmp!wizard!streaming'
APPLY     = r'/wizard/vserver/streaming/apply'

EXTENSIONS = 'mp3,ogv,flv,mov,ogg,mp4,webm'

CONFIG = """
%(rule_pre)s!match = extensions
%(rule_pre)s!match!extensions = %(extensions)s
%(rule_pre)s!handler = streaming
%(rule_pre)s!handler!rate = 1
%(rule_pre)s!handler!rate_factor = 0.5
%(rule_pre)s!handler!rate_boost = 5
"""

class Commit:
    def Commit_Rule (self):
        vsrv_num = CTK.cfg.get_val ('%s!vsrv_num'%(PREFIX))
        vsrv_pre = 'vserver!%s'%(vsrv_num)
        prio, rule_pre = cfg_vsrv_rule_get_next (vsrv_pre)
        extensions = EXTENSIONS

        config = CONFIG % (locals())

        CTK.cfg.apply_chunk (config)

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(vsrv_pre))

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()

    def __call__ (self):
        if CTK.post.pop('final'):
            CTK.cfg_apply_post()
            return self.Commit_Rule()

        return CTK.cfg_apply_post()


class Welcome:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('streaming', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += Wizard.CookBookBox ('cookbook_streaming')

        # Send the VServer num
        vsrv_num = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)[0]
        submit = CTK.Submitter (APPLY)
        submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), vsrv_num)
        submit += CTK.Hidden('final', '1')
        cont += submit

        cont += box
        cont += NextStep (vsrv_num)
        return cont.Render().toStr()


class NextStep (CTK.Container):
    def __init__ (self, vsrv_num):
        CTK.Container.__init__ (self)

        self._pre = 'vserver!%s'%(vsrv_num)
        if self._can_proceed ():
            self += CTK.DruidButtonsPanel_Create()
        else:
            self += CTK.RawHTML ('<p>%s</p>' %(self._msg))
            self += CTK.DruidButtonsPanel_Cancel()

    def _can_proceed (self):
        mods = filter(lambda x: 'streaming' in x, os.listdir(CHEROKEE_PLUGINDIR))
        if not len(mods):
            self._msg = _("The media streaming plug-in is not installed.")
            return False

        rules = CTK.cfg.keys('%s!rule'%(self._pre))
        for r in rules:
            if CTK.cfg.get_val ('%s!rule!%s!match'%(self._pre, r)) != 'extensions':
                continue
            if CTK.cfg.get_val ('%s!rule!%s!match!extensions'%(self._pre, r)) == EXTENSIONS:
                self._msg = _("Media streaming is already configured.")
                return False
        return True


# Rule
CTK.publish ('^/wizard/vserver/(\d+)/streaming$',   Welcome)
CTK.publish (r'^%s'%(APPLY), Commit, method="POST")
