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
from Server import request

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
   if ('%(url)s'.length > 0) {
      refresh.trigger({'type':'refresh_goto', 'goto':'%(url)s'});
   } else {
      druid.trigger('druid_exiting');
   }
});

submitters.bind ('submit_fail', function (event) {
   $(this).unbind (event);
});

submitters.trigger ({'type': 'submit'});
"""

JS_BUTTON_CLOSE = """
$(this).parents('.ui-dialog-content:first').dialog('close');
return false;
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

    def JS_to_goto (self, url):
        props = {'refresh': self.refreshable.id,
                 'url':     url}
        return "$('#%(refresh)s').trigger({'type':'refresh_goto', 'goto': %(url)s});" %(props)


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

        props = {'url':       '',
                 'do_submit': 'true'}

        # Event
        self.bind ('click', JS_BUTTON_GOTO%(props))

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
    def __init__ (self, url, cancel=True, do_submit=True, props={}):
        DruidButtonsPanel.__init__ (self, props.copy())
        if cancel:
            self += DruidButton_Close(_('Cancel'))
        self += DruidButton_Goto (_('Next'), url, do_submit)

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

class DruidButtonsPanel_Create (DruidButtonsPanel):
    def __init__ (self, cancel=True, props={}):
        DruidButtonsPanel.__init__ (self, props.copy())
        if cancel:
            self += DruidButton_Close(_('Cancel'))
        self += DruidButton_Submit (_('Create'))

class DruidButtonsPanel_Cancel (DruidButtonsPanel):
    def __init__ (self, props={}):
        DruidButtonsPanel.__init__ (self, props.copy())
        self += DruidButton_Close(_('Cancel'))


#
# Helper
#
def druid_url_next (url):
    parts = url.split('/')

    try:
        num = int(parts[-1])
    except ValueError:
        return '%s/2' %(url)

    return '%s/%d' %('/'.join(parts[:-1]), num+1)

def druid_url_prev (url):
    parts = url.split('/')
    num = int(parts[-1])

    if num == 2:
        return '/'.join(parts[:-1])
    return '%s/%d' %('/'.join(parts[:-1]), num-1)


class DruidButtonsPanel_Next_Auto (DruidButtonsPanel_Next):
    def __init__ (self, **kwargs):
        kwargs['url'] = druid_url_next(request.url)
        DruidButtonsPanel_Next.__init__ (self, **kwargs)

class DruidButtonsPanel_PrevNext_Auto (DruidButtonsPanel_PrevNext):
    def __init__ (self, **kwargs):
        kwargs['url_prev'] = druid_url_prev(request.url)
        kwargs['url_next'] = druid_url_next(request.url)
        DruidButtonsPanel_PrevNext.__init__ (self, **kwargs)

class DruidButtonsPanel_PrevCreate_Auto (DruidButtonsPanel_PrevCreate):
    def __init__ (self, **kwargs):
        kwargs['url_prev'] = druid_url_prev(request.url)
        DruidButtonsPanel_PrevCreate.__init__ (self, **kwargs)
