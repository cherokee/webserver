# -*- coding: utf-8 -*-
#
# Cherokee-admin
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

import os
import copy

import CTK
import Cherokee
import SavingChecks

from configured import *

SAVED_NOTICE     = N_("Save Configuration")
SAVED_RESTART    = N_("Would you like to apply the changes to the running server now?")
SAVED_NO_RUNNING = N_("The configuration file has been saved successfully.")

URL_SAVE          = r'/save'
URL_SAVE_GRACEFUL = r'/save/apply/graceful'
URL_SAVE_HARD     = r'/save/apply/hard'
URL_SAVE_NONE     = r'/save/apply/none'

SAVE_CHECK_H1 = N_("Sanity checks")
SAVE_CHECK_P1 = N_("Glitches were found in the current configuration. Please, fix the following issues:")
SAVE_CHECK_P2 = N_("Note that the configuration file was not saved.")

HELP_HTML = """
   <div id="help">
     <a class="helpbutton" href="#" id="help-a"><span>%(help)s</span></a>
     %(helps)s
   </div>
"""

HELP_JS = """
   $('#help-a').click (function(){ toggleHelp(); });
"""

SAVE_BUTTON = """
$('#save-button').bind ('click', function(){
  /* Do nothing if it hasn't changed */
  if ($(this).hasClass('saved')) {
     return;
  }

  %(show_dialog_js)s
})"""


def Restart (mode):
    if mode == 'graceful':
        Cherokee.server.restart (graceful=True)

    elif mode == 'hard':
        Cherokee.server.restart (graceful=False)

    return {'ret': 'ok', 'not-modified': '#save-button'}


class Save:
    def __call__ (self, dialog):
        # Check
        errors = SavingChecks.check_config()
        if errors:
            # FIXME: Needs to change the dialog title to "Configuration was not saved" or something
            ul = CTK.List()
            for error in errors:
                link = CTK.Link (error.url, CTK.RawHTML(_("Solve")))
                content  = CTK.Box()
                content += CTK.RawHTML ('%s: '%(error.title))
                content += link
                ul += content

            all = CTK.Box({'id': 'sanity-msg'})
            all += CTK.RawHTML ("<h2>%s</h2>"%(SAVE_CHECK_H1))
            all += CTK.RawHTML ("<p>%s</p>"  %(SAVE_CHECK_P1))
            all += CTK.Box ({'id': "sanity-errors"}, ul)
            all += CTK.RawHTML ("<p>%s</p>"  %(SAVE_CHECK_P2))

            render = all.Render()
            return render.toStr()

        # Save
        CTK.cfg.save()

        all = CTK.Box ({'id': "buttons"})
        all += CTK.RawHTML ("<h2>%s</h2>" %(_("Configuration successfully saved")))

        if Cherokee.server.is_alive():
            # Prompt about the reset
            all += CTK.RawHTML ('<p>%s</p>' %(_(SAVED_RESTART)))

            submit = CTK.Submitter (URL_SAVE_NONE)
            submit += CTK.Hidden('none', 'foo')
            submit += CTK.SubmitterButton (_('Do not restart'))
            submit.bind ('submit_success', dialog.JS_to_close())
            all += submit

            submit = CTK.Submitter (URL_SAVE_GRACEFUL)
            submit += CTK.Hidden('graceful', 'foo')
            submit += CTK.SubmitterButton (_('Graceful restart'))
            submit.bind ('submit_success', dialog.JS_to_close())
            all += submit

            submit = CTK.Submitter (URL_SAVE_HARD)
            submit += CTK.Hidden('hard', 'foo')
            submit += CTK.SubmitterButton (_('Hard restart'))
            submit.bind ('submit_success', dialog.JS_to_close())
            all += submit
        else:
            # Close the dialog
            all += CTK.RawHTML ('<p>%s</p>' %(_(SAVED_NO_RUNNING)))

            submit = CTK.Submitter (URL_SAVE_NONE)
            submit += CTK.Hidden('none', 'foo')
            submit.bind ('submit_success', dialog.JS_to_close())

            all += submit
            all += CTK.RawHTML (js = submit.JS_to_submit())

        render = all.Render()
        return render.toStr()


class Base (CTK.Page):
    def __init__ (self, title, headers=[], body_id=None, **kwargs):
        # Look for the theme file
        srcdir = os.path.dirname (os.path.realpath (__file__))
        theme_file = os.path.join (srcdir, 'theme.html')

        # Set up the template
        template = CTK.Template (filename = theme_file)
        template['title']     = title
        template['save']      = _('Save')
        template['home']      = _('Home')
        template['status']    = _('Status')
        template['general']   = _('General')
        template['vservers']  = _('vServers')
        template['sources']   = _('Sources')
        template['advanced']  = _('Advanced')
        template['help']      = _('Help')
        template['updating']  = _('Updating...')
        template['HELP_HTML'] = kwargs.has_key('helps') and HELP_HTML or ''
        template['HELP_JS']   = kwargs.has_key('helps') and HELP_JS   or ''

        # <body> property
        if body_id:
            template['body_props'] = ' id="body-%s"' %(body_id)

        # Save dialog
        dialog = CTK.DialogProxyLazy (URL_SAVE, {'title': _(SAVED_NOTICE), 'autoOpen': False, 'draggable': False, 'width': 500})
        CTK.publish (URL_SAVE, Save, dialog=dialog)

        # Default headers
        heads = copy.copy (headers)
        heads.append ('<link rel="stylesheet" type="text/css" href="/static/css/cherokee-admin.css" />')

        # Help translation
        if kwargs.has_key('helps'):
            helps = [(x[0], _(x[1])) for x in kwargs['helps']]
            kwargs['helps'] = helps

        # Parent's constructor
        CTK.Page.__init__ (self, template, heads, **kwargs)

        # Add the 'Save' dialog
        js = SAVE_BUTTON %({'show_dialog_js': dialog.JS_to_show()})
        if CTK.cfg.has_changed():
            js += ".removeClass('saved');"
        else:
            js += ".addClass('saved');"

        self += dialog
        self += CTK.RawHTML (js=js)


CTK.publish (URL_SAVE_GRACEFUL, Restart, mode='graceful')
CTK.publish (URL_SAVE_HARD,     Restart, mode='hard')
CTK.publish (URL_SAVE_NONE,     Restart, mode='none')
