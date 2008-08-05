import cgi
from urllib import unquote

class Post:
    def __init__ (self, raw=''):
        self._vars = {}

        tmp = cgi.parse_qs (raw, keep_blank_values=1)
        for key in tmp:
            self._vars[key] = []
            for n in range(len(tmp[key])):
                value = tmp[key][n]                
                self._vars[key] += [unquote (value)]
    
    def _smart_chooser (self, key):
        if not key in self._vars:
            return None

        vals = filter(lambda x: len(x)>0, self._vars[key])
        if not len(vals) > 0:
            return None

        return vals[0]

    def get_val (self, key, not_found=None):
        tmp = self._smart_chooser(key)
        if not tmp: 
            return not_found
        return tmp

    def pop (self, key, not_found=None):
        val = self._smart_chooser(key)
        if not val: 
            return not_found
        if key in self._vars:
            del(self[key])
        return val

    # Relay on the internal array methods
    #
    def __getitem__ (self, key):
        return self._vars[key]

    def __setitem__ (self, key, val):
        self._vars[key] = val
    
    def __delitem__ (self, key):
        del (self._vars[key])
        
    def __len__ (self):
        return len(self._vars)

    def __iter__ (self):
        return iter(self._vars)

    def __str__ (self):
        return str(self._vars)
