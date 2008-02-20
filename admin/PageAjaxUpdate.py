from Form import *

class PageAjaxUpdate (WebComponent):
    def __init__ (self, cfg):
        WebComponent.__init__ (self, 'page', cfg)

    def _op_render (self):
        return "ok"

    def _op_handler (self, uri, post):
        for confkey in post:
            if not '!' in confkey:
                continue

            value = post[confkey][0]
            if not value:
                del (self._cfg[confkey])
            else:
                self._cfg[confkey] = value

            return "ok"
