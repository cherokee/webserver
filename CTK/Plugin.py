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

__author__ = 'Alvaro Lopez Ortega <alvaro@alobbs.com>'

import os
import sys
import traceback

from consts import *
from Widget import Widget
from Container import Container
from Combobox import ComboCfg
from Server import cfg, publish, post
from PageCleaner import Postprocess

SELECTOR_CHANGED_JS = """
$('#%(id)s').bind ('change', this, function() {
    info = {'%(key)s': $('#%(id)s')[0].value };
    $.ajax ({url:      '%(url)s',
             type:     'POST',
	     async:     true,
	     data:      info,
             success:  function(data) {
                 $('#%(plugin_id)s').html(data);
             },
             error: function (xhr, ajaxOptions, thrownError) {
		 alert ("Error: " + xhr.status +"\\n"+ xhr.statusText);
             }
    });
});
"""


class Plugin (Container):
    def __init__ (self, key, **kwargs):
        Container.__init__ (self, **kwargs)

        self.key = key
        self.id  = "Plugin_%s" %(self.uniq_id)

class PluginInstanceProxy:
    def __call__ (self, key, modules):
        # Update the configuration
        new_val = post.get_val (key, None)
        cfg[key] = new_val

        # Instance the content
        plugin = instance_plugin (new_val, key)
        if not plugin:
            return ''

        # Render it
        render = plugin.Render()

        output  = '<div id="%s">%s</div>' %(plugin.id, render.html)
        output += HTML_JS_ON_READY_BLOCK %(render.js)

        return Postprocess(output)


class PluginSelector (Container):
    def __init__ (self, key, modules):
        Container.__init__ (self)

        # Properties
        self._key  = key
        self._mods = modules
        self._url  = '/plugin_content_%d' %(self.uniq_id)
        name       = cfg.get_val (self._key)

        # Widgets
        self.selector_widget = ComboCfg (key, modules)
        self.plugin          = instance_plugin (name, key)

        # Register hidden URL for the plugin content
        publish (self._url, PluginInstanceProxy, key=key, modules=modules, method='POST')

    def Render (self):
        # Load the plugin
        render = self.plugin.Render()

        # Warp the content
        render.html = '<div id="%s">%s</div>' %(self.plugin.id, render.html)

        # Add the initialization Javascript
        render.js += SELECTOR_CHANGED_JS %({
                'id':        self.selector_widget.id,
                'url':       self._url,
                'plugin_id': self.plugin.id,
                'key':       self._key})

        return render


# Helper functions
#

def load_module (name):
    # Sanity check
    if not name:
        return

    # Shortcut: Is it already loaded?
    if sys.modules.has_key(name):
        return sys.modules[name]

    # Figure the path to admin's python source
    stack    = traceback.extract_stack()
    first_py = stack[0][0]
    mod_path = os.path.abspath(os.path.join (first_py, '../plugins'))

    # Load the plug-in
    sys.path.append (mod_path)
    try:
        plugin = __import__(name)
        sys.modules[plugin.__name__] = plugin
    finally:
        del sys.path[-1]

    # Got it
    return plugin


def instance_plugin (name, key):
    # Load the Python module
    module = load_module (name)
    if not module:
        # Instance an empty plugin
        plugin = Plugin(key)
        return plugin

    # Instance an object
    class_name = 'Plugin_%s' %(name)
    obj = module.__dict__[class_name](key)
    return obj
