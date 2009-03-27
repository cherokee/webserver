from Form import *
from Table import *
from Module import *
from validations import *
from consts import *

from ModuleHandler import *
from ModuleBalancer import NOTE_BALANCER

NOTE_REUSE_MAX       = "Maximum number of connection per server that the proxy can try to keep opened."
NOTE_ALLOW_KEEPALIVE = "Allow the server to use Keep-alive connections with the back-end servers."

HELPS = [
    ('modules_handlers_proxy', "Reverse Proxy")
]

class ModuleProxy (ModuleHandler):
    PROPERTIES = [
        'balancer', 'in_allow_keepalive',
        'in_header_add',      'out_header_add',
        'in_header_hide',     'out_header_hide',
        'in_rewrite_request', 'out_rewrite_request'
    ]

    def __init__ (self, cfg, prefix, submit):
        ModuleHandler.__init__ (self, 'proxy', cfg, prefix, submit)
        self.show_document_root = False

    def _op_render (self):
        txt = ''

        txt += '<h2>Reverse Proxy Settings</h2>'
        txt += self.Indent (self._render_general())

        txt += '<h2>Request</h2>'
        txt += self.Indent (self._render_request())
        txt += '<h2>Reply</h2>'
        txt += self.Indent (self._render_reply())

        # Balancers
        table = TableProps()
        prefix = "%s!balancer" % (self._prefix)
        e = self.AddPropOptions_Reload (table, "Balancer", prefix,
                                        modules_available(BALANCERS), NOTE_BALANCER)

        txt += '<h2>Back-end Servers</h2>'
        txt += self.Indent(str(table) + e)

        return txt

    def _render_general (self):
        table = TableProps()
        self.AddPropEntry (table, 'Reuse connections', '%s!reuse_max'%(self._prefix), NOTE_REUSE_MAX)
        self.AddPropCheck (table, 'Allow Keepalive',   '%s!in_allow_keepalive'%(self._prefix), True, NOTE_ALLOW_KEEPALIVE)
        return str(table)

    def _render_request (self):
        txt  = ''
        txt += self._render_generic_url_rewrite ("in_rewrite_request",
                                                 "URL Rewriting")

        txt += self._render_generic_header_list ("in_header_add",
                                                 "tmp!in_header_add", 
                                                 "Header Addition",
                                                 "Add New Header")

        txt += self._render_generic_header_removal ("in_header_hide",
                                                    "Hide Headers",
                                                    "Hide Header")
        return txt

    def _render_reply (self):
        txt  = ''
        txt += self._render_generic_url_rewrite ("out_rewrite_request",
                                                 "URL Rewriting")

        txt += self._render_generic_header_list ("out_header_add",
                                                 "tmp!out_header_add", 
                                                 "Header addition",
                                                 "Add New Header")

        txt += self._render_generic_header_removal ("out_header_hide",
                                                    "Hide Headers",
                                                    "Hide Header")
        return txt


    def _render_generic_header_list (self, key, tmp_key, h3_title, title_new):
        tmp  = '<h3>%s</h3>' % (h3_title)
        keys = self._cfg.keys("%s!%s"%(self._prefix, key))
        if keys:
            table = Table(3,1, style='width="90%"')
            table += ('Header', 'Value', '')

            for k in keys:
                pre = '%s!%s!%s'%(self._prefix, key, k)
                val = self.InstanceEntry (pre, 'text', size=40)

                js      = "post_del_key('/ajax/update', '%s');" % (pre)
                rm_link = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                table += (k, val, rm_link)

            tmp += self.Indent (table)

        pre   = "%s!%s"%(self._prefix, key)
        key   = self.InstanceEntry ("%s_key"%(tmp_key), 'text', size=20, req=True)
        val   = self.InstanceEntry ("%s_val"%(tmp_key), 'text', size=40, req=True)

        table  = Table(3,1)
        table += (title_new, 'Value', '')
        table += (key, val, SUBMIT_ADD)
        tmp += self.Indent(table)
        return tmp

    def _render_generic_header_removal (self, key, h3_title, title_new):
        tmp  = '<h3>%s</h3>' % (h3_title)
        keys = self._cfg.keys("%s!%s"%(self._prefix, key))
        if keys:
            table = Table(2,1, style='width="90%"')
            table += ('Header', '')

            for k in keys:
                pre = '%s!%s!%s'%(self._prefix, key, k)
                hdr = self._cfg.get_val (pre)

                js      = "post_del_key('/ajax/update', '%s');" % (pre)
                rm_link = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                table += (hdr, rm_link)
            tmp += self.Indent (table)

            tmp2 = [int(x) for x in keys]
            tmp2.sort()
            next = tmp2[-1]+1
        else:
            next = 1

        pre   = "%s!%s"%(self._prefix, key)
        hdr_e = self.InstanceEntry ("%s!%d"%(pre,next), 'text', size=40)

        table  = Table(2,1)
        table += (title_new, '')
        table += (hdr_e, SUBMIT_ADD)
        tmp += self.Indent(table)
        return tmp

    def _render_generic_url_rewrite (self, key, title):
        tmp = '<h3>%s</h3>'%(title)
        keys = self._cfg.keys("%s!%s"%(self._prefix, key))
        if keys:
            table = Table(3,1, style='width="90%"')
            table += ('Regular Expression', 'Substitution', '')

            for k in keys:
                pre = '%s!%s!%s'%(self._prefix, key, k)
                regex = self._cfg.get_val ('%s!regex'%(pre))
                subst = self._cfg.get_val ('%s!substring'%(pre))

                js      = "post_del_key('/ajax/update', '%s');" % (pre)
                rm_link = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                table += (regex, subst, rm_link)

            tmp += self.Indent (table)

            tmp2 = [int(x) for x in keys]
            tmp2.sort()
            next = tmp2[-1]+1
        else:
            next = 1

        pre   = "%s!%s"%(self._prefix, key)
        regex = self.InstanceEntry ("%s!%d!regex"%(pre,next),     'text', size=20, req=True)
        subst = self.InstanceEntry ("%s!%d!substring"%(pre,next), 'text', size=40, req=True)

        table  = Table(3,1)
        table += ('Add RegEx', 'Substitution', '')
        table += (regex, subst, SUBMIT_ADD)
        tmp += self.Indent(table)
        return tmp

    def _op_apply_changes (self, uri, post):
        for t in ['in', 'out']:
            # A new header is being added
            n_header = post.pop('tmp!%s_header_add_key'%(t))
            n_hvalue = post.pop('tmp!%s_header_add_val'%(t))

            del (self._cfg['tmp!%s_header_add_key'%(t)])
            del (self._cfg['tmp!%s_header_add_val'%(t)])

            if n_header:
                self._cfg["%s!%s_header_add!%s"%(self._prefix, t, n_header)] = n_hvalue

        # Apply balancer changes
        pre  = "%s!balancer" % (self._prefix)

        new_balancer = post.pop(pre)
        if new_balancer:
            self._cfg[pre] = new_balancer

        cfg  = self._cfg[pre]
        if cfg and cfg.value:
            name = cfg.value
            props = module_obj_factory (name, self._cfg, pre, self.submit_url)
            props._op_apply_changes (uri, post)

        # And CGI changes
        self.ApplyChangesPrefix (self._prefix, ['in_allow_keepalive'], post)
