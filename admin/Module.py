import imp, sys
from CherokeeManagement import cherokee_has_plugin

class Module:
    def __init__ (self, id, cfg, prefix, submit_url):
        self._id        = id
        self._cfg       = cfg
        self._prefix    = prefix
        self.submit_url = submit_url

def module_obj_factory_detailed (mod_type, name, cfg, prefix, submit_url, **kwargs):
    # Assemble module name
    mod_name = reduce (lambda x,y: x+y, map(lambda x: x.capitalize(), name.split('_')))

    # Load the module source file
    mod = imp.load_source (name, "%s%s.py" % (mod_type, mod_name))
    sys.modules[name] = mod

    # Instance the object
    src = "mod_obj = mod.%s%s(cfg, prefix, submit_url)" % (mod_type, mod_name)
    exec(src)

    # Add properties
    for prop in kwargs:
        mod_obj.__dict__[prop] = kwargs[prop]

    return mod_obj

def module_get_help (name):
    if not name:
        return []
    mod_name = reduce (lambda x,y: x+y, map(lambda x: x.capitalize(), name.split('_')))
    mod = __import__("Module%s" % (mod_name))
    return getattr (mod, "HELPS", [])


def module_obj_factory (name, cfg, prefix, submit_url, **kwargs):
    return module_obj_factory_detailed ("Module", name, cfg, prefix, submit_url, **kwargs)

def modules_available (module_list):
    new_module_list = []

    for entry in module_list:
        assert (type(entry) == tuple)
        assert (len(entry) == 2)
        plugin, name = entry

        if not len(plugin) or \
            cherokee_has_plugin (plugin):
            new_module_list.append(entry)

    return new_module_list
