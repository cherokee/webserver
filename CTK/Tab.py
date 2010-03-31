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

from Widget import Widget

HEADER = [
    '<link type="text/css" href="/CTK/css/CTK.css" rel="stylesheet" />',
    '<script type="text/javascript" src="/CTK/js/jquery-ui-1.7.2.custom.min.js"></script>',
    '<script type="text/javascript" src="/CTK/js/jquery.cookie.js"></script>'
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
function set_tab_cookie (value) {
    var path_begin = location.href.indexOf('/', location.href.indexOf('://') + 3);
    var path       = location.href.substring (path_begin);

    $.cookie ('open_tab', value, {path: path});
}

$("#tab_%(id)s").tabs().bind('tabsselect', function(event, ui) {
    tabslen = $("#tab_%(id)s").tabs('length');

    nprevtab = parseInt ($.cookie('open_tab')) + 2;
    if (nprevtab < tabslen) {
    	$("#tab_%(id)s li:nth-child("+ nprevtab  +")").removeClass("ui-tabs-selected-next");
    } else {
    	$("#tab_%(id)s li:nth-child("+ nprevtab  +")").removeClass("ui-tabs-selected-next-last");
    }

    nnexttab = ui.index + 2;
    if (nnexttab < tabslen) {
        $("#tab_%(id)s li:nth-child("+ nnexttab  +")").addClass("ui-tabs-selected-next");
    } else {
        $("#tab_%(id)s li:nth-child("+ nnexttab  +")").addClass("ui-tabs-selected-next-last");
    }

    set_tab_cookie (ui.index);
});

$("#tab_%(id)s ul li:first").addClass("ui-tabs-first");
$("#tab_%(id)s ul li:last").addClass("ui-tabs-last");

var open_tab = $.cookie('open_tab');
if (open_tab) {
    $("#tab_%(id)s").tabs("select", parseInt(open_tab));
} else {
    set_tab_cookie ("0");
}

if ($("#tab_%(id)s").tabs('option', 'selected') == 0) {
    if ($("#tab_%(id)s").tabs('length') == 2) {
        $("#tab_%(id)s li:nth-child(2)").addClass("ui-tabs-selected-next-last");
    } else {
        $("#tab_%(id)s li:nth-child(2)").addClass("ui-tabs-selected-next");
    }
}
"""

class Tab (Widget):
    def __init__ (self, props=None):
        Widget.__init__ (self)
        self._tabs  = []

        if props:
            self._props = props
        else:
            self._props = {}

        if not 'id' in self._props:
            self._props['id'] = 'widget%d'%(self.uniq_id)

    def Add (self, title, widget):
        assert type(title) == str
        assert isinstance(widget, Widget)

        self._tabs.append ((title, widget))

    def Render (self):
        render = Widget.Render(self)
        id     = self._props['id']

        ul_html  = ''
        tab_html = ''

        num  = 1
        for title, widget in self._tabs:
            r = widget.Render()

            # Keep record of dependencies
            render.js      += r.js
            render.headers += r.headers
            render.helps   += r.helps

            props = {'id':     id,
                     'widget': r.html,
                     'title':  title,
                     'num':    num}

            # Render <ul>
            ul_html  += HTML_LI  %(props)
            tab_html += HTML_TAB %(props)

            num  += 1

        # Render the whole thing
        tmp  = HTML_UL %({'li_tabs': ul_html})
        tmp += tab_html

        html = HTML %({'id':   id,
                       'html': tmp})

        props = {'id':   id,
                 'tabs': html}

        render.html     = html
        render.js      += JS_INIT %(props)
        render.headers += HEADER

        return render
