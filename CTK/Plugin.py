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
import string
import traceback

from consts import *
from Widget import Widget
from Container import Container
from Combobox import ComboCfg
from Server import cfg, publish, post
from PageCleaner import Postprocess
from Help import HelpEntry, HelpGroup

SELECTOR_CHANGED_JS = """
/* On selector change
 */
$('#%(id)s').bind ('change', this, function() {
    info = {'%(key)s': $('#%(id)s')[0].value };
    $.ajax ({url:      '%(url)s',
             type:     'POST',
	     async:     true,
	     data:      info,
             success:  function(data) {
                 $('#%(plugin_id)s').html(data);
		 $('#%(id)s').trigger('changed');
             },
             error: function (xhr, ajaxOptions, thrownError) {
		 alert ("Error: " + xhr.status +"\\n"+ xhr.statusText);
             }
    });

   /* Update the Help menu
    */
   Help_update_group ('%(key)s', $('#%(id)s')[0].value);
});

/* Help: Initial status
 */
Help_update_group ('%(key)s', $('#%(id)s')[0].value);
"""


class Plugin (Container):
    def __init__ (self, key):
        Container.__init__ (self)

        self.key = key
        self.id  = "Plugin_%s" %(self.uniq_id)

    def apply (self):
        "Commodity method"
        for key in post:
            cfg[key] = post[key]
        return {'ret': 'ok'}


class PluginInstanceProxy:
    def __call__ (self, key, modules, **kwargs):
        # Update the configuration
        new_val = post.get_val (key, None)
        if not new_val:
            return ''

        cfg[key] = new_val

        # Instance the content
        plugin = instance_plugin (new_val, key, **kwargs)
        if not plugin:
            return ''

        # Render it
        render = plugin.Render()

        output  = '<div id="%s">%s</div>' %(plugin.id, render.html)
        output += HTML_JS_ON_READY_BLOCK %(render.js)

        return Postprocess(output)


class PluginSelector (Widget):
    def __init__ (self, key, modules, **kwargs):
        def key_to_url (key):
            return ''.join ([('_',c)[c in string.letters + string.digits] for c in key])

        Widget.__init__ (self)

        # Properties
        self._key   = key
        self._mods  = modules
        self._url   = '/plugin_content_%s' %(key_to_url(key))
        active_name = cfg.get_val (self._key)

        # Widgets
        self.selector_widget = ComboCfg (key, modules)
        self.plugin          = instance_plugin (active_name, key, **kwargs)

        # Register hidden URL for the plugin content
        publish (self._url, PluginInstanceProxy, key=key, modules=modules, method='POST', **kwargs)

    def _get_helps (self):
        global_key  = self._key.replace('!','_')
        global_help = HelpGroup(global_key)

        for e in self._mods:
            name, desc = e
            module = load_module (name)
            if module:
                if 'HELPS' in dir(module):
                    help_grp = HelpGroup (name)
                    for entry in module.HELPS:
                        help_grp += HelpEntry (entry[1], entry[0])
                    global_help += help_grp

        return [global_help]

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

        # Helps
        render.helps = self._get_helps()
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

    # Figure the path to admin's python source (hacky!)
    stack = traceback.extract_stack()
    for stage in stack:
        if 'CTK/Server.py' in stage[0]:
            first_py = stage[0]
            break

    mod_path = os.path.abspath(os.path.join (first_py, '../../../plugins'))

    # Load the plug-in
    sys.path.append (mod_path)
    try:
        plugin = __import__(name)
        sys.modules[plugin.__name__] = plugin
    finally:
        del sys.path[-1]

    # Got it
    return plugin


def instance_plugin (name, key, **kwargs):
    # Load the Python module
    module = load_module (name)
    if not module:
        # Instance an empty plugin
        plugin = Plugin(key)
        return plugin

    # Instance an object
    class_name = 'Plugin_%s' %(name)
    obj = module.__dict__[class_name](key, **kwargs)
    return obj
