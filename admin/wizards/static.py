# -*- coding: utf-8 -*-
#
# Cherokee-admin's Common Static wizard
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
# 2010/06/15: Cherokee 1.0.3b
#

import re
import CTK
import Wizard
from util import *
from configured import *

NOTE_WELCOME_H1 = N_("Welcome to the Common Static wizard")
NOTE_WELCOME_P1 = N_("This wizard adds a rule to optimally serve the most common static files.")
NOTE_CREATE_H1  = N_("Current rules have been checked")
NOTE_CREATE_OK  = N_("The process is very simple. Let the wizard take over and don't worry about a thing.")
NOTE_CREATE_ERR = N_("Common files have been already configured, so there is nothing to be done.")

PREFIX    = 'tmp!wizard!static'
URL_APPLY = r'/wizard/vserver/static/apply'

class Commit:
    def Commit_Rule (self):
        vsrv_num = CTK.cfg.get_val ('%s!vsrv_num'%(PREFIX))
        vsrv_pre = 'vserver!%s' %(vsrv_num)
        x, rule_pre = cfg_vsrv_rule_get_next (vsrv_pre)
        Wizard.AddUsualStaticFiles (rule_pre)

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(vsrv_pre))

        del (CTK.cfg[PREFIX])
        return CTK.cfg_reply_ajax_ok()


    def __call__ (self):
        if CTK.post.pop('final'):
            CTK.cfg_apply_post()
            return self.Commit_Rule()

        return CTK.cfg_apply_post()


class Create:
    def _check_if_valid (self):
        vsrv_num = CTK.cfg.get_val ('%s!vsrv_num'%(PREFIX))
        vsrv_pre = 'vserver!%s' %(vsrv_num)
        rules = CTK.cfg.keys('%s!rule'%(vsrv_pre))
        for r in rules:
            if CTK.cfg.get_val ('%s!rule!%s!match'%(vsrv_pre, r)) == 'fullpath':
                files   = Wizard.USUAL_STATIC_FILES[:]
                entries = CTK.cfg.keys('%s!rule!%s!match!fullpath'%(vsrv_pre, r))
                for e in entries:
                    f = CTK.cfg.get_val('%s!rule!%s!match!fullpath!%s'%(vsrv_pre, r, e))
                    try:
                        files.remove(f)
                    except ValueError:
                        pass
                if not len(files):
                    return False
        return True

    def __call__ (self):
        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>'%(_(NOTE_CREATE_H1)))
        cont += submit

        if self._check_if_valid ():
            cont += CTK.RawHTML ('<p>%s</p>'%(_(NOTE_CREATE_OK)))
            cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        else:
            cont += CTK.RawHTML ('<p>%s</p>'%(_(NOTE_CREATE_ERR)))
            cont += CTK.DruidButtonsPanel_Cancel()

        return cont.Render().toStr()


class Welcome:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('static', {'class': 'wizard-descr'})
        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        cont += box

        # Send the VServer num
        tmp = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)
        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('%s!vsrv_num'%(PREFIX), tmp[0])
        cont += submit
        cont += CTK.DruidButtonsPanel_Next_Auto()
        return cont.Render().toStr()


# Rule
CTK.publish ('^/wizard/vserver/(\d+)/static$',  Welcome)
CTK.publish ('^/wizard/vserver/(\d+)/static/2', Create)
CTK.publish (r'^%s'%(URL_APPLY), Commit, method="POST")
