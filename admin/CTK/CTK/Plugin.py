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

import os
import sys
import imp
import string
import traceback

from consts import *
from Widget import Widget
from Container import Container
from Combobox import ComboCfg
from Server import cfg, publish, post, get_server
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

                 /* Activate the Save button */
                 var save_button = $('#save-button');
                 save_button.show();
                 save_button.removeClass('saved');
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


class PluginInstanceProxy:
    def __call__ (self, key, modules, **kwargs):
        # Update the configuration
        if not key in post.keys():
            return ''

        new_val = post.get_val (key)
        cfg[key] = new_val

        if not new_val:
            return ''

        # Instance the content
        plugin = instance_plugin (new_val, key, **kwargs)
        if not plugin:
            return ''

        # Render it
        render = plugin.Render()

        output  = '<div id="%s">%s</div>' %(plugin.id, render.html)
        if render.js:
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
        active_name = cfg.get_val (self._key)

        # URL
        self._url = '/plugin_content_%s' %(key_to_url(key))

        srv = get_server()
        if srv.use_sec_submit:
            self._url += '?key=%s' %(srv.sec_submit)

        # Widgets
        self.selector_widget = ComboCfg (key, modules)
        self.plugin          = instance_plugin (active_name, key, **kwargs)

        # Register hidden URL for the plugin content
        publish (r'^/plugin_content_%s' %(key_to_url(key)), PluginInstanceProxy,
                 key=key, modules=modules, method='POST', **kwargs)

    def _get_helps (self):
        global_key  = self._key.replace('!','_')
        global_help = HelpGroup(global_key)

        for e in self._mods:
            name, desc = e
            module = load_module (name, 'plugins')
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
def load_module_pyc (fullpath_pyc, namespace, use_cache=True, load_src=True):
    files = [fullpath_pyc]

    # Load source if present
    if load_src and \
       (fullpath_pyc.endswith('.pyc') or \
        fullpath_pyc.endswith('.pyo')):
        files.insert (0, fullpath_pyc[:-1])

    # Load the first available
    for fullpath in files:
        # Cache
        if use_cache:
            if sys.modules.has_key (namespace):
                if sys.modules[namespace].__file__ == fullpath:
                    return sys.modules[namespace]

        # Load
        if os.path.exists (fullpath):
            if fullpath.endswith ('.py'):
                return imp.load_source (namespace, fullpath)
            return imp.load_compiled (namespace, fullpath)


def unload_module (name):
    if name in sys.modules:
        del (sys.modules[name])


def load_module (name, dirname):
    # Sanity check
    if not name:
        return

    # Check the different plug-in dirs
    srv = get_server()

    for path in srv.plugin_paths:
        mod_path = os.path.abspath (os.path.join (path, dirname))
        fullpath = os.path.join (mod_path, "%s.py"%(name))

        if os.access (fullpath, os.R_OK):
            break

    # Shortcut: it might be loaded
    if sys.modules.has_key (name):
        loaded_mod_file = sys.modules[name].__file__
        if loaded_mod_file.endswith('.pyc'):
            loaded_mod_file = loaded_mod_file[:-1]

        if loaded_mod_file == fullpath:
            return sys.modules[name]

    # Load the plug-in
    fullpath = os.path.join (mod_path, "%s.py"%(name))

    try:
        return imp.load_source (name, fullpath)
    except IOError:
        print "Could not load '%s'." %(fullpath)
        raise


def instance_plugin (name, key, **kwargs):
    # Load the Python module
    module = load_module (name, 'plugins')
    if not module:
        # Instance an empty plugin
        plugin = Plugin(key)
        return plugin

    # Instance an object
    class_name = 'Plugin_%s' %(name)
    obj = module.__dict__[class_name](key, **kwargs)
    return obj


def figure_plugin_paths():
    paths = []

    # Figure the path to admin's python source (hacky!)
    stack = traceback.extract_stack()

    if 'pyscgi.py' in ' '.join([x[0] for x in stack]):
        for stage in stack:
            if 'CTK/pyscgi.py' in stage[0]:
                paths.append (os.path.join (stage[0], '../../..'))
                break

    paths.append (os.path.dirname (stack[0][0]))
    return paths
