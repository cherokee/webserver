# -*- coding: utf-8 -*-
#
# Cherokee-admin's Icons' wizard
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
# 2010/04/13: Cherokee 0.99.41
#

import re
import CTK
import Wizard
from util import *
from configured import *

NOTE_WELCOME_H1 = N_("Welcome to the Icons Wizard")
NOTE_WELCOME_P1 = N_("This wizard adds the /cherokee_icons and /cherokee_themes directories so Cherokee can use icons when listing directories.")
NOTE_WELCOME_ERR= N_("The /cherokee_icons and /cherokee_themes directories are already configured. There is nothing left to do.")

PREFIX    = 'tmp!wizard!icons'
URL_APPLY = r'/wizard/vserver/icons/apply'

CONFIG_ICONS = """
%(rule_pre_2)s!match = directory
%(rule_pre_2)s!match!directory = /cherokee_icons
%(rule_pre_2)s!handler = file
%(rule_pre_2)s!handler!iocache = 1
%(rule_pre_2)s!document_root = %(droot_icons)s
%(rule_pre_2)s!encoder!gzip = 0
%(rule_pre_2)s!encoder!deflate = 0
%(rule_pre_2)s!expiration = time
%(rule_pre_2)s!expiration!time = 1h
"""

CONFIG_THEMES = """
%(rule_pre_1)s!match = directory
%(rule_pre_1)s!match!directory = /cherokee_themes
%(rule_pre_1)s!handler = file
%(rule_pre_1)s!handler!iocache = 1
%(rule_pre_1)s!document_root = %(droot_themes)s
%(rule_pre_1)s!encoder!gzip = 0
%(rule_pre_1)s!encoder!deflate = 0
%(rule_pre_1)s!expiration = time
%(rule_pre_1)s!expiration!time = 1h
"""

class Commit:
    def Commit_Rule (self):
        vsrv_num = CTK.cfg.get_val ('%s!vsrv_num'%(PREFIX))
        icons    = bool(int(CTK.cfg.get_val ('%s!icons'%(PREFIX))))
        themes   = bool(int(CTK.cfg.get_val ('%s!themes'%(PREFIX))))

        vsrv_pre = 'vserver!%s' %(vsrv_num)
        rule_n, x = cfg_vsrv_rule_get_next (vsrv_pre)

        config_src = ''
        if not icons:
            config_src += CONFIG_ICONS
        if not themes:
            config_src += CONFIG_THEMES

        rule_pre_1   = '%s!rule!%d'%(vsrv_pre, rule_n)
        rule_pre_2   = '%s!rule!%d'%(vsrv_pre, rule_n+1)
        droot_icons  = CHEROKEE_ICONSDIR
        droot_themes = CHEROKEE_THEMEDIR

        config = config_src % (locals())
        CTK.cfg.apply_chunk (config)

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(vsrv_pre))

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()

    def __call__ (self):
        CTK.cfg_apply_post()
        return self.Commit_Rule()


class Welcome:
    def _check_config (self):
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)[0]
        vsrv_pre = 'vserver!%s'
        rules = CTK.cfg.keys('%s!rule'%(vsrv_pre))
        icons, themes = False, False
        for r in rules:
            if CTK.get_val ('%s!rule!%s!match'%(vsrv_pre, r)) == 'directory' and \
               CTK.cfg.get_val ('%s!rule!%s!match!directory'%(vsrv_pre, r)) == '/cherokee_icons':
                icons = True
            if CTK.cfg.get_val ('%s!rule!%s!match'%(vsrv_pre, r)) == 'directory' and \
               CTK.cfg.get_val ('%s!rule!%s!match!directory'%(vsrv_pre, r)) == '/cherokee_themes':
                themes = True

        return (icons, themes)

    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('icons', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        cont += box

        icons, themes = self._check_config()
        if False in [icons, themes]:
            # Send the VServer num
            tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
            submit += CTK.Hidden('%s!icons'%(PREFIX),  ('0','1')[icons])
            submit += CTK.Hidden('%s!themes'%(PREFIX), ('0','1')[themes])
            submit += CTK.Hidden('final', '1')
            cont += submit
            cont += CTK.DruidButtonsPanel_Create()
        else:
            cont += CTK.RawHTML ('<p>%s</p>'   %(_(NOTE_WELCOME_ERR)))
            cont += CTK.DruidButtonsPanel_Cancel()

        return cont.Render().toStr()


# Rule
CTK.publish ('^/wizard/vserver/(\d+)/icons$',  Welcome)
CTK.publish (r'^%s'%(URL_APPLY), Commit, method="POST")
