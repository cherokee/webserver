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
        str_class = ''
        if 'req' in kwargs and kwargs['req'] == True:
            str_class += 'required '
            del kwargs['req']

        if 'noautosubmit' in kwargs and kwargs['noautosubmit'] == True:
            str_class += 'noautosubmit '
            del kwargs['noautosubmit']

        if str_class:
            self._kwargs['class'] = str_class

    def _init_value (self, cfg):
        try:
            value = cfg[self._name].value
            self._kwargs['value'] = value
        except:
            pass

    def __str__ (self):
        suffix = ""
        disabled = ""

        if self._type == "checkbox":
            if "quiet" in self._kwargs and self._kwargs["quiet"]:
                del(self._kwargs["quiet"])
            else:
                suffix = self.AddToggleSpan(" Enabled")

        if "disabled" in self._kwargs:
            if self._kwargs["disabled"] == True:
                disabled = " disabled"

            del (self._kwargs["disabled"])

        error = '<div id="error_%s"></div>' % (self._name)
        props = 'id="%s" name="%s" type="%s"%s' % (self._name, self._name, self._type, disabled)

        for prop in self._kwargs:
            props += ' %s="%s"' % (prop, self._kwargs[prop])

        return "<input %s />"%(props) + suffix + error

    def AddToggleSpan (self, text):
        js = "javascript:var obj = get_by_id('%s'); obj.checked = (! obj.checked); do_autosubmit(obj);" % (self._name)
        text = '<span class="cbtoggletext" onClick="%s">%s</span>' % (js, text)
        return text


class EntryOptions:
    def __init__ (self, name, options, *args, **kwargs):
        self._name     = name
        self._opts     = options
        self._args     = args
        self._kwargs   = kwargs
        self._selected = None

        if 'noautosubmit' in kwargs and kwargs['noautosubmit'] == True:
            self._kwargs['class'] = 'noautosubmit '
            del kwargs['noautosubmit']

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
