import re

_global_file_cache = {}

class Theme:
    def __init__ (self, name):
        template = "%s.template.html" % (name)
        self._template = self._read_file (template)

    # Public Methods
    #
    def BuildTemplate (self, keys, id):
        render = self._template
        while '%(' in render:
            for replacement in re.findall (r'\%\((\w+)\)s', render):
                macro = '%('+replacement+')s'
                render = render.replace (macro, keys[replacement])

        return render

    def ReadFile (self, name):
        filename = "%s.html" % (name)
        return self._read_file (filename)

    # Private
    #
    def _read_file (self, filename):
        if filename in _global_file_cache:
            file = _global_file_cache[filename]
        else:
            file = open (filename,'r').read()
            _global_file_cache[filename] = file
        return file


    def __str__ (self):
        return self._raw
