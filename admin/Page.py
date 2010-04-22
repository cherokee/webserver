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

import os
import copy

import CTK
import Cherokee
import configured

SAVED_NOTICE     = N_("The Configuration has been Saved")
SAVED_RESTART    = N_("Would you like to apply the changes to the running server now?")
SAVED_NO_RUNNING = N_("The configuration file has been saved successfuly.")

URL_SAVE          = r'/save'
URL_SAVE_GRACEFUL = r'/save/apply/graceful'
URL_SAVE_HARD     = r'/save/apply/hard'
URL_SAVE_NONE     = r'/save/apply/none'

SAVE_BUTTON = """
$('#save-button').bind ('click', function(){
  /* Do nothing if it hasn't changed */
  if ($(this).hasClass('saved')) {
     return;
  }

  %s
})"""


def Restart (mode):
    if mode == 'graceful':
        Cherokee.server.restart (graceful=True)

    elif mode == 'hard':
        Cherokee.server.restart (graceful=False)

    return {'ret': 'ok', 'not-modified': '#save-button'}


class Save:
    def __call__ (self, dialog):
        # Save
        CTK.cfg.save()

        all = CTK.Box({'id': "buttons"})

        if Cherokee.server.is_alive():
            # Prompt about the reset
            all += CTK.RawHTML ('<p>%s</p>' %(SAVED_RESTART))

            submit = CTK.Submitter (URL_SAVE_NONE)
            submit += CTK.Hidden('none')
            submit += CTK.SubmitterButton (_('Do not restart'))
            submit.bind ('submit_success', dialog.JS_to_close())
            all += submit

            submit = CTK.Submitter (URL_SAVE_GRACEFUL)
            submit += CTK.Hidden('graceful')
            submit += CTK.SubmitterButton (_('Graceful restart'))
            submit.bind ('submit_success', dialog.JS_to_close())
            all += submit

            submit = CTK.Submitter (URL_SAVE_HARD)
            submit += CTK.Hidden('hard')
            submit += CTK.SubmitterButton (_('Hard restart'))
            submit.bind ('submit_success', dialog.JS_to_close())
            all += submit
        else:
            # Prompt about the reset
            all += CTK.RawHTML ('<p>%s</p>' %(SAVED_NO_RUNNING))

            submit = CTK.Submitter (URL_SAVE_NONE)
            submit += CTK.Hidden('none')
            submit += CTK.SubmitterButton (_('OK'))
            submit.bind ('submit_success', dialog.JS_to_close())
            all += submit

        render = all.Render()
        return render.toStr()


class Base (CTK.Page):
    def __init__ (self, title, headers=[], body_id=None, **kwargs):
        # Look for the theme file
        srcdir = os.path.dirname (os.path.realpath (__file__))
        theme_file = os.path.join (srcdir, 'theme.html')

        # Set up the template
        template = CTK.Template (filename = theme_file)
        template['title'] = title

        # <body> property
        if body_id:
            template['body_props'] = ' id="body-%s"' %(body_id)

        # Save dialog
        dialog = CTK.DialogProxyLazy (URL_SAVE, {'title': _(SAVED_NOTICE), 'autoOpen': False, 'draggable': False, 'width': 500})
        CTK.publish (URL_SAVE, Save, dialog=dialog)

        # Default headers
        heads = copy.copy (headers)
        heads.append ('<link rel="stylesheet" type="text/css" href="/static/css/cherokee-admin.css" />')

        # Parent's constructor
        CTK.Page.__init__ (self, template, heads, **kwargs)

        # Add the 'Save' dialog
        js = SAVE_BUTTON %(dialog.JS_to_show())
        if CTK.cfg.has_changed():
            js += ".removeClass('saved');"
        else:
            js += ';'

        self += dialog
        self += CTK.RawHTML (js=js)


CTK.publish (URL_SAVE_GRACEFUL, Restart, mode='graceful')
CTK.publish (URL_SAVE_HARD,     Restart, mode='hard')
CTK.publish (URL_SAVE_NONE,     Restart, mode='none')
