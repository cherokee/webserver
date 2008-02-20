import imp, sys

class Module:
    def __init__ (self, id, cfg, prefix, submit_url):
        self._id        = id
        self._cfg       = cfg
        self._prefix    = prefix
        self.submit_url = submit_url

def module_obj_factory (name, cfg, prefix, submit_url, **kwargs):
    # Assemble module name
    mod_name = reduce (lambda x,y: x+y, map(lambda x: x.capitalize(), name.split('_')))

    # Load the module source file
    mod = imp.load_source (name, "Module%s.py" % (mod_name))
    sys.modules[name] = mod

    # Instance the object
    src = "mod_obj = mod.Module%s(cfg, prefix, submit_url)" % (mod_name)
    exec(src)

    # Add properties
    for prop in kwargs:
        mod_obj.__dict__[prop] = kwargs[prop]

    return mod_obj
