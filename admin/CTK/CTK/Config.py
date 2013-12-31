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

import copy
import types

class ConfigNode (object):
    def __init__ (self):
        self._val   = None
        self._child = {}

    # Value
    #
    def _get_value (self):
        return self._val
    def _set_value (self, val):
        self._val = val

    value = property (_get_value, _set_value)

    def get_val (self, key, default=None):
        try:
            subcfg = self[key]
        except:
            return default
        if not subcfg:
            return default
        return subcfg.value

    def get_vals (self, keys, default=None):
        values = []
        for k in keys:
            values += [self.get_val(k)]
        return values

    # Get child
    #
    def _getitem_simple (self, key):
        if not key in self._child:
            return None

        r = self._child[key]
        assert (isinstance(r, ConfigNode))
        return r

    def _getitem_complex (self, path):
        node = self
        for p in path.split('!'):
            tmp = node[p]
            if not tmp:
                return None
            node = tmp
        return node

    def __getitem__ (self, path):
        if '!' in path:
            return self._getitem_complex (path)
        else:
            return self._getitem_simple (path)

    # Add children
    #
    def _create_path (self, path):
        node = self
        for p in path.split('!'):
            tmp = node[p]
            if not tmp:
                tmp = ConfigNode()
                node[p] = tmp
            node = tmp
        return node

    def _setitem_simple (self, key, val):
        assert (isinstance(val, ConfigNode))
        self._child[key] = val

    def __setitem__ (self, key, val):
        if '!' in key:
            obj = self[key]
            if not obj:
                self._create_path(key)

        if isinstance (val, ConfigNode):
            if '!' in key:
                parts = key.split('!')
                last  = parts[-1]
                prev  = reduce (lambda x,y: "%s!%s" % (x,y), parts[:-1])
                obj = self[prev]
                obj._child[last] = val
            else:
                self._child[key] = val
        else:
            if '!' in key:
                self[key].value = val
            else:
                child = self[key]
                if not child:
                    child = self._create_path (key)
                child.value = val

    # Modify child
    #
    def __delitem__ (self, key):
        del (self._child[key])

    def __iter__ (self):
        return iter(self._child)

    # Serialize
    def serialize (self, path=''):
        content = ''
        if self._val is not None:
            if type(self._val) == types.BooleanType:
                val = str(int(self._val))
            else:
                val = str(self._val)
            content += '%s = %s\n' % (path, val)

        for name in self._child:
            if name == 'tmp':
                continue

            node = self._child[name]
            if len(path) != 0:
                new_path = "%s!%s" % (path, name)
            else:
                new_path = name

            content += node.serialize (new_path)
        return content

    # Sub-tree operations
    def set_value (self, key, val):
        if not key in self._child:
            cfg = ConfigNode()
            self._child[key] = cfg
        else:
            cfg = self._child[key]

        cfg.value = val

    def has_child (self):
        return len(self._child) > 0

    def keys (self):
        return self._child.keys()

    def __contains__ (self, key):
        return key in self._child.keys()

    def __cmp__ (self, other):
        if self._val != other._val:
            return cmp (self._val, other._val)

        for k in self._child:
            if not k in other:
                return 1
            re = cmp(self._child[k], other[k])
            if re != 0:
                return re

        for k in other._child:
            if not k in self:
                return -1
            re = cmp(other[k], self._child[k])
            if re != 0:
                return re

        return 0


class Config:
    """
    Class to represent and interact with the configuration tree. An
    optional configuration file can be passed as argument on
    instantiation to load and parse an initial configuration tree.
    """
    def __init__ (self, file=None):
        self.root      = ConfigNode()
        self.root_orig = None
        self.file      = file

        # Build ConfigNode tree
        if file:
            self.load()

    def load (self):
        """Load and Parse the configuration file specified on
        initialization."""
        # Load and Parse
        try:
            f = open (self.file, "r")
        except:
            pass
        else:
            self._parse (f.read())
            f.close()

        # Original copy
        self.root_orig = copy.deepcopy (self.root)

    def _create_path (self, path):
        node = self.root
        for p in path.split('!'):
            tmp = node[p]
            if not tmp:
                tmp = ConfigNode()
                node[p] = tmp
            node = tmp
        return node

    def _parse (self, config_string):
        for line in config_string.split('\n'):
            node = self.root

            while len(line) > 1 and line[-1] in " \r\n\t":
                line = line[:-1]

            if len(line) < 5: continue
            if line[0] == '#': continue

            try:
                path, value = line.split (" = ", 1)
            except:
                msg = _("ERROR: Couldn't unpack '%s'")
                print msg % (line)
                raise

            node = self._create_path (path)
            node.value = value

    def __str__ (self):
        return self.root.serialize()

    # Access
    def __getitem__ (self, path):
        return self.root[path]

    def clone (self, path_old, path_new):
        """Clone a given branch into another location in the
        configuration tree."""
        parent, parent_path, child_name = self._get_parent_node (path_old)
        if self.root[path_new]:
            return True
        copied = copy.deepcopy (self[path_old])
        self.set_sub_node (path_new, copied)

    def rename (self, path_old, path_new):
        """Rename a branch in the configuration tree given by path_old
        to a new name given by path_new."""
        error = self.clone (path_old, path_new)
        if not error:
            del(self[path_old])
        return error

    def set_sub_node (self, path, config_node):
        assert isinstance(config_node, ConfigNode), type(config_node)

        # Top level entry
        if not '!' in path:
            self.root[path] = config_node
            return

        # Deep entries
        parent, parent_path, child_name = self._get_parent_node (path)
        if not parent:
            # Target parent does not exist
            self._create_path (parent_path)
            parent, parent_path, child_name = self._get_parent_node (path)

        if not parent._child.has_key(child_name):
            parent._create_path(child_name)

        parent._child[child_name] = config_node

    def __setitem__ (self, path, val):
        if not isinstance (val, ConfigNode):
            if not val or len(val) == 0:
                del (self[path])
                return

        tmp = self[path]
        if not tmp:
            tmp = self._create_path (path)

        tmp.value = val

    def _get_parent_node (self, path):
        preg = path.split('!')
        last = preg[-1]
        pres = reduce (lambda x,y: "%s!%s" % (x,y), preg[:-1])
        return (self[pres], pres, last)

    def get_val (self, path, default=None):
        """Return the value of a configuration entry given by the
        'path' argument. If not found, return the value of the
        optional 'default' argument."""
        return self.root.get_val (path, default)

    def get_vals (self, paths, default=None):
        return self.root.get_vals (paths, default)

    def __delitem__ (self, path):
        if '!' in path:
            parent, parent_path, child_name = self._get_parent_node (path)
            if parent and parent._child.has_key(child_name):
                del (parent._child[child_name])
        else:
            if path in self.root.keys():
                del (self.root[path])

    def keys (self, path):
        """Show keys for a given configuration path."""
        tmp = self[path]
        if not tmp:
            return []
        return tmp.keys()

    def pop (self, key):
        """Pop an element from the configuration tree given by
        argument key."""
        tmp = self.get_val(key)
        del (self[key])
        return tmp

    # Serialization
    def serialize (self):
        """Dump the contents of the configuration tree into a
        string."""
        def sorter(x,y):
            order = ['config', 'server', 'vserver', 'source', 'icons', 'mime', 'admin']
            a = x.split('!')
            b = y.split('!')
            try:
                ai = order.index(a[0])
                bi = order.index(b[0])
            except:
                return cmp(x,y)

            # Different tags
            if ai > bi:
                return  1
            elif ai < bi:
                return -1

            # Sort rules: reverse
            if ((len(a) > 3) and
                (a[0] == b[0] == 'vserver') and
                (a[1] == b[1]) and
                (a[2] == b[2] == 'rule')):
                re = cmp (int(b[3]), int(a[3]))
                if re != 0:
                    return re

            return cmp(x,y)

        tmp = self.root.serialize().split('\n')
        tmp.sort(sorter)
        return '\n'.join (filter (lambda x: len(x) > 1, tmp))

    def save (self):
        """Save current configuration tree into the file specified on
        initialization. Try to make a copy with the '.backup'
        extension if possible."""
        # Try to make a copy
        try:
            t = open (self.file+'.backup', 'w+')
            s = open (self.file, 'r')
            t.write (s.read())
            t.close()
            s.close()
        except:
            print _("Could not copy configuration to ") + self.file + '.backup'

        # Write the new one
        cfg = self.serialize()

        t = open (self.file, 'w+')
        t.write (cfg)
        t.close()

        # Update the original tree
        self.root_orig = copy.deepcopy (self.root)

    # Checks
    def is_writable (self):
        """Check if configuration file is writeable."""
        import os

        if not os.path.exists (self.file):
            return False
        return os.access (self.file, os.W_OK)

    def has_tree (self):
        """Check if configuration tree exists."""
        return len(self.root._child) > 0

    # Utilities
    def get_next_entry_prefix (self, pre):
        """Return next element to insert into a configuration node
        given by the 'pre' prefix."""
        entries = [int(x) for x in self.keys(pre)]
        entries.sort()

        if not entries:
            return '%s!1'%(pre)

        return '%s!%d'%(pre, entries[-1] + 1)

    def get_lowest_entry (self, pre):
        """Return first existing element of a configuration node given
        by the 'pre' prefix."""
        entries = [int(x) for x in self.keys(pre)]
        entries.sort()

        if entries:
            return entries[0]

        return 1

    def normalize (self, pre, step=10):
        """Reenumerate the branch given by the 'pre' prefix for
        normalization. An optional step can be provided."""
        keys = self.keys(pre)
        keys.sort (lambda x,y: cmp(int(x),int(y)))

        del (self['tmp!normalize'])

        n = step
        for k in keys:
            self.clone ('%s!%s'%(pre, k), 'tmp!normalize!%s!%s'%(pre,n))
            n += step

        del (self[pre])
        if self['tmp!normalize']:
            self.rename ('tmp!normalize!%s'%(pre), pre)

    def apply_chunk (self, chunk):
        """Insert a configuration chunk into the configuration tree,
        all at once. This is an in-memory operation. Manual steps must
        be taken to update the configuration file if needed."""
        lines = [l.strip() for l in chunk.split('\n')]
        lines = filter (lambda x: len(x) and x[0] != '#', lines)

        for line in lines:
            left, right = line.split (" = ", 2)
            self[left] = right

    def has_changed (self):
        """Check if configuration tree differs from the last one that
        was saved to the configuration file."""
        root = copy.deepcopy (self.root)
        orig = copy.deepcopy (self.root_orig)

        if not root and orig:
            return True
        if not orig and root:
            return True

        if 'tmp' in root:
            del (root['tmp'])
        if 'tmp' in orig:
            del (orig['tmp'])

        return not root == orig
