# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2009-2014 Alvaro Lopez Ortega
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
from Table import Table
from Widget import Widget
from RawHTML import RawHTML
from Submitter import Submitter
from Container import Container
from HiddenField import HiddenField
from util import *

HTML_TABLE = """
<div class="propstable">%s</div>
"""

HEADERS = ['<link rel="stylesheet" type="text/css" href="/CTK/css/CTK.css" />']


class PropsTableEntry (Box):
    """Property Table Entry"""

    def __init__ (self, title, widget, comment, props_={}):
        self.title   = title
        self.widget  = widget
        self.comment = comment

        # Properties
        props = props_.copy()

        if 'id' in props:
            self.id = props.pop('id')

        if 'class' in props:
            props['class'] += ' entry'
        else:
            props['class'] = 'entry'

        # Constructor
        Box.__init__ (self, props)

        # Compose
        self += Box ({'class': 'title'}, RawHTML(self.title))

        if self.widget:
            self += Box ({'class': 'widget'}, widget)
        else:
            self += Box ({'class': 'widget'}, Container())

        if isinstance(comment, Widget):
            self += Box ({'class': 'comment'}, comment)
        else:
            self += Box ({'class': 'comment'}, RawHTML(comment))

        self += RawHTML('<div class="after"></div>')


class PropsTable (Box):
    """
    Property Table: Displays a succession of rows consisting of title,
    widget, and comment. Dynamically grows the number of rows as
    necessary.

    Example:
       table = CTK.PropsTable()
       table.Add ('My title1', CTK.TextField({'name': 'var1'}), 'First variable')
       table.Add ('My title2', CTK.TextField({'name': 'var2'}), 'Second variable')
    """
    def __init__ (self, **kwargs):
        Box.__init__ (self, {'class': "propstable"})

    def Add (self, title, widget, comment):
        self += PropsTableEntry (title, widget, comment)


class PropsAuto (PropsTable):
    """
    Automatic Property Table: Displays a succession of rows consisting
    of title, widget, and comment. Dynamically grows the number of
    rows as necessary. It must be instanced with a URL to assign it to
    an underlying Submitter element, that will send HTTP POST requests
    when input elements are changed. This is done automatically.

    Arguments:
       url: the URL to receive the HTTP POST requests.

    Example:
       table = CTK.PropsAuto()

       table.Add ('My title1',
                  CTK.TextField({'name': 'var1'}),
                  'First variable')

       table.Add ('My title2',
                  CTK.TextField({'name': 'var2'}),
                  'Second variable. This is never sent.',
                  use_submitter=False)
    """
    def __init__ (self, url, **kwargs):
        PropsTable.__init__ (self, **kwargs)
        self.url       = url
        self.constants = {}

    def Add (self, title, widget, comment, use_submitter=True):
        if use_submitter:
            submit = Submitter (self.url)
        else:
            submit = Container()

        submit += widget

        # Add constants
        for key in self.constants:
            submit += HiddenField ({'name': key, 'value': self.constants[key]})

        # Append the widget
        PropsTable.Add (self, title, submit, comment)
