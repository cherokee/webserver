# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2009 Alvaro Lopez Ortega
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

from Widget import Widget, RenderResponse

HEADERS = [
    '<link type="text/css" href="/static/css/jquery-ui-1.7.2.custom.css" rel="stylesheet" />',
    '<script type="text/javascript" src="/static/js/common.js"></script>',
    '<script type="text/javascript" src="/static/js/jquery-ui-1.7.2.custom.min.js"></script>'
]

HTML = """
<div id="tab_%(id)s">
 %(html)s
</div> <!-- %(id)s -->
"""

HTML_UL = """<ul class="ui-tabs-nav">%(li_tabs)s</ul>"""
HTML_LI = """<li><a href="#tabs_%(id)s-%(num)d"><span>%(title)s</span></a></li>"""

HTML_TAB = """
<div id="tabs_%(id)s-%(num)d">
  %(widget)s
</div> <!-- tabs_%(id)s-%(num)d -->
"""

JS_INIT = """
$("#tab_%(id)s").tabs().bind('tabsselect', function(event, ui) {
    document.cookie = "open_tab="  + ui.index;
});

var open_tab = get_cookie('open_tab');
if (open_tab) {
      $("#tab_%(id)s").tabs("select", open_tab);
}
"""

class Tab (Widget):
    def __init__ (self, props={}):
        Widget.__init__ (self)
        self._props = props
        self._tabs  = []

        if not 'id' in props:
            self._props['id'] = 'widget%d'%(self.uniq_id)

    def Add (self, title, widget):
        assert type(title) == str
        assert isinstance(widget, Widget)

        self._tabs.append ((title, widget))

    def Render (self):
        id     = self._props['id']
        render = RenderResponse()

        ul_html  = ''
        tab_html = ''

        num  = 1
        for title, widget in self._tabs:
            r = widget.Render()

            # Keep record of dependencies
            render.js      += r.js
            render.headers += r.headers

            props = {'id':     id,
                     'widget': r.html,
                     'title':  title,
                     'num':    num}

            # Render <ul>
            ul_html  += HTML_LI  %(props)
            tab_html += HTML_TAB %(props)

            num  += 1

        # Rende the whole thing
        tmp  = HTML_UL %({'li_tabs': ul_html})
        tmp += tab_html

        html = HTML %({'id':   id,
                       'html': tmp})

        props = {'id':   id,
                 'tabs': html}

        render.html     = html
        render.js      += JS_INIT %(props)
        render.headers += HEADERS

        return render
