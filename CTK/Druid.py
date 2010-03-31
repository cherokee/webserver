# -*- coding: utf-8 -*-
#
# CTK: Cherokee Toolkit
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

from Box import Box
from Button import Button
from Refreshable import RefreshableURL


JS_BUTTON_GOTO = """
var druid      = $(this).parents('.druid:first');
var refresh    = druid.find('.refreshable-url');
var submitters = refresh.find('.submitter');


// No Submit
if ((! %(do_submit)s) || (submitters.length == 0))
{
   refresh.trigger({'type':'refresh_goto', 'goto':'%(url)s'});
   return;
}

// Submit
submitters.bind ('submit_success', function (event) {
   $(this).unbind (event);
   refresh.trigger({'type':'refresh_goto', 'goto':'%(url)s'});
});

submitters.bind ('submit_fail', function (event) {
   $(this).unbind (event);
});

submitters.trigger ({'type': 'submit'});
"""

JS_BUTTON_CLOSE = """
console.log("dialog", $(this).parents('.ui-dialog:first'));
$(this).parents('.ui-dialog:first').dialog().dialog('close');
return false;
"""

JS_BUTTON_SUBMIT = """
$(this)
.trigger ({type: 'submit'})
.parents('.ui-dialog:first').dialog().dialog('close');
"""


class Druid (Box):
    def __init__ (self, refreshable, _props={}):
        # Properties
        props = _props.copy()
        if 'class' in props:
            props['class'] += ' druid'
        else:
            props['class'] = 'druid'

        # Parent's constructor
        Box.__init__ (self, props)

        # Widget
        assert isinstance (refreshable, RefreshableURL)
        self.refreshable = refreshable
        self += self.refreshable


#
# Buttons
#

class DruidButton (Button):
    def __init__ (self, caption, url, _props={}):
        # Properties
        props = _props.copy()
        if 'class' in props:
            props['class'] += ' druid-button'
        else:
            props['class'] = 'druid-button'

        # Parent's constructor
        Button.__init__ (self, caption, props.copy())

class DruidButton_Goto (DruidButton):
    def __init__ (self, caption, url, do_submit, _props={}):
        DruidButton.__init__ (self, caption, url, _props.copy())

        props = {'url':       url,
                 'do_submit': ('false', 'true')[do_submit]}

        # Event
        self.bind ('click', JS_BUTTON_GOTO %(props))

class DruidButton_Close (DruidButton):
    def __init__ (self, caption, _props={}):
        DruidButton.__init__ (self, caption, _props.copy())

        # Event
        self.bind ('click', JS_BUTTON_CLOSE)

class DruidButton_Submit (DruidButton):
    def __init__ (self, caption, _props={}):
        DruidButton.__init__ (self, caption, _props.copy())

        # Event
        self.bind ('click', JS_BUTTON_SUBMIT)


#
# Button Panels
#

class DruidButtonsPanel (Box):
    def __init__ (self, _props={}):
        # Properties
        props = _props.copy()
        if 'class' in props:
            props['class'] += ' druid-button-panel'
        else:
            props['class'] = 'druid-button-panel'

        # Parent's constructor
        Box.__init__ (self, props)
        self.buttons = []

class DruidButtonsPanel_Next (DruidButtonsPanel):
    def __init__ (self, url, cancel=True, props={}):
        DruidButtonsPanel.__init__ (self, props.copy())
        if cancel:
            self += DruidButton_Close(_('Cancel'))
        self += DruidButton_Goto (_('Next'), url, False)

class DruidButtonsPanel_PrevNext (DruidButtonsPanel):
    def __init__ (self, url_prev, url_next, cancel=True, do_submit=True, props={}):
        DruidButtonsPanel.__init__ (self, props.copy())
        if cancel:
            self += DruidButton_Close(_('Cancel'))
        self += DruidButton_Goto (_('Next'), url_next, do_submit)
        self += DruidButton_Goto (_('Prev'), url_prev, False)

class DruidButtonsPanel_PrevCreate (DruidButtonsPanel):
    def __init__ (self, url_prev, cancel=True, props={}):
        DruidButtonsPanel.__init__ (self, props.copy())
        if cancel:
            self += DruidButton_Close(_('Cancel'))
        self += DruidButton_Submit (_('Create'))
        self += DruidButton_Goto (_('Prev'), url_prev, False)


