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


class Config:
    def __init__ (self, file=None):
        self.root = ConfigNode()
        self.file = file

        # Build ConfigNode tree
        if file:
            try:
                f = open (file, "r")
            except:
                pass
            else:
                self._parse (f.read())
                f.close()

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

            path, value = line.split (" = ")
            node = self._create_path (path)
            node.value = value
            
    def __str__ (self):
        return self.root.serialize()

    # Access
    def __getitem__ (self, path):
        return self.root[path]

    def clone (self, path_old, path_new):
        parent, parent_path, child_name = self._get_parent_node (path_old)
        if self.root[path_new]:
            return True
        copied = copy.deepcopy (self[path_old])
        self.set_sub_node (path_new, copied)
    
    def rename (self, path_old, path_new):
        error = self.clone (path_old, path_new)
        if not error:
            del(self[path_old])
        return error

    def set_sub_node (self, path, config_node):
        assert (isinstance(config_node, ConfigNode))

        parent, parent_path, child_name = self._get_parent_node (path)
        if parent:
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
        return self.root.get_val (path, default)

    def __delitem__ (self, path):
        parent, parent_path, child_name = self._get_parent_node (path)
        if parent and parent._child.has_key(child_name):
            del (parent._child[child_name])

    def keys (self, path):
        tmp = self[path]
        if not tmp:
            return []
        return tmp.keys()

    def pop (self, key):
        tmp = self.get_val(key)
        del (self[key])
        return tmp

    # Serialization
    def serialize (self):
        def sorter(x,y):
            order = ['server', 'vserver', 'source', 'icons', 'mime']
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
        return '\n'.join(tmp)

    def save (self):
        # Try to make a copy
        try:
            t = open (self.file+'.backup', 'w')
            s = open (self.file, 'r')
            t.write (s.read())
            t.close()
            s.close()
        except:
            print ("Could copy configuration to " + self.file+'.backup')

        # Write the new one
        cfg = self.serialize()

        t = open (self.file, 'w')
        t.write (cfg)
        t.close()

    # Checks
    def is_writable (self):
        import os

        if not os.path.exists (self.file):
            return False
        return os.access (self.file, os.W_OK)

    def has_tree (self):
        return len(self.root._child) > 0

