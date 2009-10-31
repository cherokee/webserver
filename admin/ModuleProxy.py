from Form import *
from Table import *
from Module import *
from validations import *
from consts import *

# For gettext
N_ = lambda x: x

from ModuleHandler import *
from ModuleBalancer import NOTE_BALANCER

NOTE_REUSE_MAX       = N_("Maximum number of connection per server that the proxy can try to keep opened.")
NOTE_ALLOW_KEEPALIVE = N_("Allow the server to use Keep-alive connections with the back-end servers.")
NOTE_PRESERVE_HOST   = N_("Preserve the original \"Host:\" header sent by the client. (Default No)")
NOTE_PRESERVE_SERVER = N_("Preserve the \"Server:\" header sent by the back-end server. (Default No)")

HELPS = [
    ('modules_handlers_proxy', N_("Reverse Proxy"))
]

class ModuleProxy (ModuleHandler):
    PROPERTIES = [
        'balancer', 
        'in_allow_keepalive',
        'in_preserve_host',
        'out_preserve_server',
        'in_header_add',      'out_header_add',
        'in_header_hide',     'out_header_hide',
        'in_rewrite_request', 'out_rewrite_request'
    ]

    def __init__ (self, cfg, prefix, submit):
        ModuleHandler.__init__ (self, 'proxy', cfg, prefix, submit)
        self.show_document_root = False

    def _op_render (self):
        txt = ''

        txt += '<h2>%s</h2>' % (_('Reverse Proxy Settings'))
        txt += self.Indent (self._render_general())

        txt += '<h2>%s</h2>' % (_('Request'))
        txt += self.Indent (self._render_request())
        txt += '<h2>%s</h2>' % (_('Reply'))
        txt += self.Indent (self._render_reply())

        # Balancers
        table = TableProps()
        prefix = "%s!balancer" % (self._prefix)
        e = self.AddPropOptions_Reload_Module (table, _("Balancer"), prefix,
                                               modules_available(BALANCERS), _(NOTE_BALANCER))

        txt += '<h2>%s</h2>' % (_('Back-end Servers'))
        txt += self.Indent(str(table) + e)

        return txt

    def _render_general (self):
        table = TableProps()
        self.AddPropEntry (table, _('Reuse connections'),      '%s!reuse_max'%(self._prefix), _(NOTE_REUSE_MAX))
        self.AddPropCheck (table, _('Allow Keepalive'),        '%s!in_allow_keepalive'%(self._prefix), True, _(NOTE_ALLOW_KEEPALIVE))
        self.AddPropCheck (table, _('Preserve Host header'),   '%s!in_preserve_host'%(self._prefix), False, _(NOTE_PRESERVE_HOST))
        self.AddPropCheck (table, _('Preserve Server header'), '%s!out_preserve_server'%(self._prefix), False, _(NOTE_PRESERVE_SERVER))
        return str(table)

    def _render_request (self):
        txt  = ''
        txt += self._render_generic_url_rewrite ("in_rewrite_request",
                                                 _("URL Rewriting"))

        txt += self._render_generic_header_list ("in_header_add",
                                                 "tmp!in_header_add", 
                                                 _("Header Addition"),
                                                 _("Add New Header"))

        txt += self._render_generic_header_removal ("in_header_hide",
                                                    _("Hide Headers"),
                                                    _("Hide Header"))
        return txt

    def _render_reply (self):
        txt  = ''
        txt += self._render_generic_url_rewrite ("out_rewrite_request",
                                                 _("URL Rewriting"))

        txt += self._render_generic_header_list ("out_header_add",
                                                 "tmp!out_header_add", 
                                                 _("Header addition"),
                                                 _("Add New Header"))

        txt += self._render_generic_header_removal ("out_header_hide",
                                                    _("Hide Headers"),
                                                    _("Hide Header"))
        return txt


    def _render_generic_header_list (self, key, tmp_key, h3_title, title_new):
        tmp  = '<h3>%s</h3>' % (h3_title)
        keys = self._cfg.keys("%s!%s"%(self._prefix, key))
        if keys:
            table = Table(3,1, style='width="90%"')
            table += (_('Header'), _('Value'), '')

            for k in keys:
                pre = '%s!%s!%s'%(self._prefix, key, k)
                val = self.InstanceEntry (pre, 'text', size=40)

                rm_link = self.AddDeleteLink ('/ajax/update', pre)
                table += (k, val, rm_link)

            tmp += self.Indent (table)

        pre   = "%s!%s"%(self._prefix, key)
        key   = self.InstanceEntry ("%s_key"%(tmp_key), 'text', size=20, req=True)
        val   = self.InstanceEntry ("%s_val"%(tmp_key), 'text', size=40, req=True)

        table  = Table(3,1)
        table += (title_new, _('Value'), '')
        table += (key, val, SUBMIT_ADD)
        tmp += self.Indent(table)
        return tmp

    def _render_generic_header_removal (self, key, h3_title, title_new):
        tmp  = '<h3>%s</h3>' % (h3_title)
        keys = self._cfg.keys("%s!%s"%(self._prefix, key))
        if keys:
            table = Table(2,1, style='width="90%"')
            table += (_('Header'), '')

            for k in keys:
                pre = '%s!%s!%s'%(self._prefix, key, k)
                hdr = self._cfg.get_val (pre)

                rm_link = self.AddDeleteLink ('/ajax/update', pre)
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
            table += (_('Regular Expression'), _('Substitution'), '')

            for k in keys:
                pre = '%s!%s!%s'%(self._prefix, key, k)
                regex = self.InstanceEntry('%s!regex'%(pre),     'text', size=30, req=False)
                subst = self.InstanceEntry('%s!substring'%(pre), 'text', size=30, req=False)
                rm_link = self.AddDeleteLink ('/ajax/update', pre)
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
        table += (_('Add RegEx'), _('Substitution'), '')
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
        self.ApplyChangesPrefix (self._prefix, 
                                 ['in_allow_keepalive', 'in_preserve_host',
                                  'out_preserve_server'], post)
