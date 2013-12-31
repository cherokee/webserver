# -*- coding: utf-8 -*-
#
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
from List import List
from Link import Link
from RawHTML import RawHTML

HEADERS = [
    '<script type="text/javascript" src="/CTK/js/Carousel.js"></script>'
]

JS_INIT = """
$("#%(id)s").Carousel();
"""

class Carousel (Box):
    """
    Widget to render a slideshow box.

        Optional arguments:
            props: dictionary with properties for HTML element, such as
                {'class': 'test', 'id': 'snapshots'}

        Example:
            shots = CTK.Carousel()
            shots += CTK.Image ({'src': 'image1.png'})
            shots += CTK.Image ({'src': 'image2.png'})
    """
    def __init__ (self, props_={}):
        props = props_.copy()
        if 'class' in props:
            props['class'] += " carousel"
        else:
            props['class'] = "carousel"

        Box.__init__ (self, props.copy())
        self.images   = List ({'class': 'overview'})
        self.pager    = List ({'class': 'pager'})
        self.controls = None

        Box.__iadd__ (self, self.images)

    def __iadd__ (self, widget):
        link = Link (None, RawHTML ("%s" %(len(self.images.child) +1)))

        self.images += widget
        self.pager  += link
        self.pager[-1].props['class'] = 'pagenum'

        return self

    def Render (self):
        # Add pager and arrows if there is more than 1 item
        if len(self.pager) > 1 and not self.controls:
            arrows = Box({'class':'arrows'})
            arrows += Link (None, RawHTML("%s"%(_('left'))), {'class': "buttons prev"})
            arrows += Link (None, RawHTML("%s"%(_('right'))), {'class': "buttons next"})

            self.controls = Box({'class':'controls'})
            self.controls += arrows
            self.controls += self.pager

            Box.__iadd__ (self, self.controls)

        # Render
        render = Box.Render (self)

        render.headers += HEADERS
        render.js      += JS_INIT %({'id': self.id})

        return render


class CarouselThumbnails (Carousel):
    """
    Widget to render a slideshow box, with thumbnails.

        Optional arguments:
            props: dictionary with properties for HTML element, such as
                {'class': 'test', 'id': 'snapshots'}

        Example:
            shots = CTK.CarouselThumbnails()
            shots += CTK.Image ({'src': 'image1.png'})
            shots += CTK.Image ({'src': 'image2.png'})
    """
    def __init__ (self, props_={}):
        Carousel.__init__ (self, props_.copy())

    def __iadd__ (self, widget):
        box  = Box ({'class': 'carousel-thumbs'})
        box += RawHTML ("%s" %(len(self.images.child) +1))
        box += Box ({'class': 'carousel_thumbs-image'}, widget)
        link = Link (None, Box ({'class': 'carousel_thumbs-link'}, box))

        self.images += widget
        self.pager  += link
        self.pager[-1].props['class'] = 'pagenum'

        return self
