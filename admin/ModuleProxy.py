from Form import *
from Table import *
from Module import *
from validations import *
from consts import *

from ModuleHandler import *
from ModuleBalancer import NOTE_BALANCER

NOTE_REUSE_MAX = "Maximum number of connection per server that the proxy can try to keep opened."

HELPS = [
    ('modules_handlers_proxy', "Reverse Proxy")
]

class ModuleProxy (ModuleHandler):
    PROPERTIES = [
        'balancer', 'rewrite_request', 'header_add', 'header_hide'
    ]

    def __init__ (self, cfg, prefix, submit):
        ModuleHandler.__init__ (self, 'proxy', cfg, prefix, submit)
        self.show_document_root = False

    def _op_render (self):
        txt = '<h2>Reverse Proxy specific</h2>'

        # Properties
        table = TableProps()
        self.AddPropEntry (table, 'Reuse connections', '%s!reuse_max'%(self._prefix), NOTE_REUSE_MAX)
        txt += self.Indent (table)

        # Add headers
        tmp  = ''
        keys = self._cfg.keys("%s!header_add"%(self._prefix))
        if keys:
            tmp += '<h3>Header addition</h3>'

            table = Table(3,1, style='width="90%"')
            table += ('Header', 'Value', '')

            for k in keys:
                pre = '%s!header_add!%s'%(self._prefix, k)
                val = self.InstanceEntry (pre, 'text', size=40)

                js      = "post_del_key('/ajax/update', '%s');" % (pre)
                rm_link = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                table += (k, val, rm_link)

            tmp += self.Indent (table)

        tmp += '<h3>Add new header</h3>'

        pre   = "%s!header_add"%(self._prefix)
        key   = self.InstanceEntry ("tmp!add_header_key", 'text', size=20, req=True)
        val   = self.InstanceEntry ("tmp!add_header_val", 'text', size=40, req=True)

        table  = Table(3,1)
        table += ('Header', 'Value', '')
        table += (key, val, SUBMIT_ADD)
        tmp += self.Indent(table)

        txt += '<h2>Header Addition</h2>'
        txt += self.Indent(tmp)

        # URL Rewriting
        tmp  = ''
        keys = self._cfg.keys("%s!rewrite_request"%(self._prefix))
        if keys:
            tmp += '<h3>URL Rewriting rules</h3>'

            table = Table(3,1, style='width="90%"')
            table += ('Regular Expression', 'Substitution', '')

            for k in keys:
                pre = '%s!rewrite_request!%s'%(self._prefix, k)
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

        tmp += '<h3>Add new URL Rewrite rules</h3>'

        pre   = "%s!rewrite_request"%(self._prefix)
        regex = self.InstanceEntry ("%s!%d!regex"%(pre,next),     'text', size=20, req=True)
        subst = self.InstanceEntry ("%s!%d!substring"%(pre,next), 'text', size=40, req=True)

        table  = Table(3,1)
        table += ('Regular Expression', 'Substitution', '')
        table += (regex, subst, SUBMIT_ADD)
        tmp += self.Indent(table)

        txt += '<h2>URL Rewriting rules</h2>'
        txt += self.Indent(tmp)

        # Hide headers
        tmp  = ''
        keys = self._cfg.keys("%s!header_hide"%(self._prefix))
        if keys:
            tmp += '<h3>Hidden headers</h3>'

            table = Table(2,1, style='width="90%"')
            table += ('Header', '')

            for k in keys:
                pre = '%s!header_hide!%s'%(self._prefix, k)
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

        tmp += '<h3>Hide a header</h3>'

        pre   = "%s!header_hide"%(self._prefix)
        hdr_e = self.InstanceEntry ("%s!%d"%(pre,next), 'text', size=40)

        table  = Table(2,1)
        table += ('Header', '')
        table += (hdr_e, SUBMIT_ADD)
        tmp += self.Indent(table)

        txt += '<h2>Hidden returned headers</h2>'
        txt += self.Indent(tmp)

        # Balancers
        table = TableProps()
        prefix = "%s!balancer" % (self._prefix)
        e = self.AddPropOptions_Reload (table, "Balancer", prefix,
                                        modules_available(BALANCERS), NOTE_BALANCER)

        txt += '<h2>Internal Servers</h2>'
        txt += self.Indent(str(table) + e)
        return txt

    def _op_apply_changes (self, uri, post):
        # A new header is being added
        n_header = post.pop('tmp!add_header_key')
        n_hvalue = post.pop('tmp!add_header_val')

        del (self._cfg['tmp!add_header_key'])
        del (self._cfg['tmp!add_header_val'])

        if n_header:
            self._cfg["%s!header_add!%s"%(self._prefix, n_header)] = n_hvalue

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
        self.ApplyChangesPrefix (self._prefix, [], post)
