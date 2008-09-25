class Entry:
    def __init__ (self, name, type, cfg=None, *args, **kwargs):
        self._name   = name
        self._type   = type
        self._args   = args
        self._kwargs = kwargs

        if cfg:
            self._init_value (cfg)

        if not 'size' in kwargs:
            self._kwargs['size'] = 40

        # Entries with req=True will be checked against
        # check_all_or_none before autosubmissions
        if 'req' in kwargs and kwargs['req']==True:
            self._kwargs['class']='required'
            del kwargs['req']

    def _init_value (self, cfg):
        try:
            value = cfg[self._name].value
            self._kwargs['value'] = value
        except:
            pass

    def __str__ (self):
        suffix = ""
        if self._type == "checkbox":
            if "quiet" in self._kwargs and self._kwargs["quiet"]:
                del(self._kwargs["quiet"])
            else:
                suffix = " Enabled"

        error = '<div id="error_%s"></div>' % (self._name)
        props = 'id="%s" name="%s" type="%s"' % (self._name, self._name, self._type)

        for prop in self._kwargs:
            props += ' %s="%s"' % (prop, self._kwargs[prop])

        return "<input %s />"%(props) + suffix + error



class EntryOptions:
    def __init__ (self, name, options, *args, **kwargs):
        self._name     = name
        self._opts     = options
        self._args     = args
        self._kwargs   = kwargs
        self._selected = None

    def __str__ (self):
        props = 'id="%s" name="%s"' % (self._name, self._name)

        for prop in self._kwargs:
            if prop == "selected":
                self._selected = self._kwargs[prop]
            else:
                props += ' %s="%s"' % (prop, self._kwargs[prop])

        txt = '<select %s>\n' % (props)
        for option in self._opts:
            name, label = option
            if self._selected == name:
                txt += '\t<option value="%s" selected>%s</option>\n' % (name, label)
            else:
                txt += '\t<option value="%s">%s</option>\n' % (name, label)

        txt += '</select>\n'

        return txt
