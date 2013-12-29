# -*- coding: utf-8 -*-
#
# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2010-2014 Alvaro Lopez Ortega
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
from Combobox import Combobox

HEADERS = [
    '<script type="text/javascript" src="/CTK/js/jquery-ui-1.7.2.custom.min.js"></script>',
    '<script type="text/javascript" src="/CTK/js/StarRating.js"></script>'
]

RATING_OPTIONS = [
    ('1', "Very poor"),
    ('2', "Not that bad"),
    ('3', "Average"),
    ('4', "Good"),
    ('5', "Excellent")
]

JS_INIT = """
$("#%(id)s").StarRating ({
   'selected': %(selected)s,
   'can_set':  %(can_set)s
});
"""

class StarRating (Box):
    """
    Widget to display a star-rating bar with 5 possible vlues
    [1-5]. Arguments are optional.

       Arguments:

           props: dictionary with additional properties for the HTML
               element, such as {'name': 'foo', 'id': 'bar', 'class':
               'baz'}. Some interesting properties are:

                   selected: Number of stars to fill, as string
                       (default, '5').
                   can_set: Boolean that indicates whether or not the
                       values can be changed by clicking on the stars.

       Examples:
          stars1 = CTK.StarRating({'can_set': False, 'selected': '1')

          submit = CTK.Submitter ('/apply')
          submit += CTK.StarRating ({'name': 'rate', 'can_set': True, 'selected': '3'})
    """
    def __init__ (self, props={}):
        Box.__init__ (self, {'class': 'star-rating'})

        assert type(props) == dict
        self.selected = props.get('selected', '5')
        self.can_set  = props.pop('can_set', False)

        if 'style' in props:
            props['style'] += ' display:none;'
        else:
            props['style'] = 'display:none;'

        combo = Combobox (props.copy(), RATING_OPTIONS)
        self += combo

    def Render (self):
        render = Box.Render(self)

        render.headers += HEADERS
        render.js      += JS_INIT %({'id':       self.id,
                                     'selected': self.selected,
                                     'can_set':  ('false','true')[self.can_set]})
        return render
