from Form import *
from Table import *
from Module import *
from flags import *

import validations

NOTE_BIND = ""

class ModuleBind (Module, FormHelper):
    validation = []

    def __init__ (self, cfg, prefix, submit_url):
        FormHelper.__init__ (self, 'bind', cfg)
        Module.__init__ (self, 'bind', cfg, prefix, submit_url)

    def _build_option_bind_num (self, num):
        port = self._cfg.get_val ("server!bind!%s!port"%(num))
        inte = self._cfg.get_val ("server!bind!%s!interface"%(num))

        if inte:
            return (num, "Port %s (%s)"%(inte, port))
        return (num, "Port %s"%(port))

    def _render_new_entry (self):
        # Build the port list
        ports = [('', "Choose")]
        for b in self._cfg.keys("server!bind"):
            tmp = self._build_option_bind_num (b)
            ports.append(tmp)

        # Render the table
        cfg_key = '%s!value'%(self._prefix)

        table = TableProps()
        self.AddPropOptions (table, 'Incoming Port', cfg_key, ports, NOTE_BIND)
        return str(table)

    def _render_modify_entry (self):
        txt     = ''
        cfg_key = '%s!bind'%(self._prefix)

        # Lits ports
        tmp = self._cfg.keys(cfg_key)
        if tmp:
            txt += '<h3>Selected Ports</h3>'
            table = Table(4, 1, style='width="100%"')
            table += ('Port', 'Bind to', 'TLS', '')
            for b in tmp:
                server_bind = self._cfg.get_val('%s!%s'%(cfg_key, b))
                port = self._cfg.get_val ("server!bind!%s!port"%(server_bind))
                bind = self._cfg.get_val ("server!bind!%s!interface"%(server_bind), '')
                tls_ = self._cfg.get_val ("server!bind!%s!tls"%(server_bind), False)
                tls  = ["No", "Yes"][int(tls_)]
                js = "post_del_key('/ajax/update', '%s!%s');"%(cfg_key, b)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                table += (port, bind, tls, link_del)
            txt += self.Indent(table)

        # Don't show port already being listened to
        left = self._cfg.keys("server!bind")
        rule_used = []
        for b in self._cfg.keys(cfg_key):
            port_num = self._cfg.get_val('%s!%s'%(cfg_key,b))
            if port_num in left:
                left.remove(port_num)

        # Find the new entry number
        tmp = [int(x) for x in self._cfg.keys(cfg_key)]
        if tmp:
            tmp.sort()
            next = str(int(tmp[-1])+1)
        else:
            next = '1'

        if left:
            txt += "<h3>Assign new port</h3>"
            ports = [('', "Choose")]
            for b in left:
                tmp = self._build_option_bind_num (b)
                ports.append (tmp)

            table = TableProps()
            self.AddPropOptions (table, 'Incoming Port', '%s!%s'%(cfg_key,next), ports, NOTE_BIND)
            txt += self.Indent(table)

        return txt

    def _op_render (self):
        if self._prefix.startswith('tmp!'):
            return self._render_new_entry()
        return self._render_modify_entry()
    
    def _op_apply_changes (self, uri, post):
        self.ApplyChangesPrefix (self._prefix, None, post)

    def apply_cfg (self, values):
        if not values.has_key('value'):
            print "ERROR, a 'value' entry is needed!"

        cfg_key = '%s!bind!1'%(self._prefix)
        bind    = values['value']

        self._cfg[cfg_key] = bind

    def get_name (self):
        tmp = []
        for b in self._cfg.keys('%s!bind'%(self._prefix)):
            real_n = self._cfg.get_val('%s!bind!%s'%(self._prefix, b))
            port = self._cfg.get_val('server!bind!%s!port'%(real_n))
            tls  = self._cfg.get_val('server!bind!%s!tls'%(real_n), False)
            tmp.append((port, bool(int(tls))))

        def render_entry (e):
            port, tls = e
            txt = port
            if tls:
                txt += ' (TLS)'
            return txt

        def sort_func(x,y):
            return cmp(int(x[0]),int(y[0]))

        tmp.sort(cmp=sort_func)
        return ", ".join([render_entry(x) for x in tmp])

    def get_type_name (self):
        return self._id.capitalize()
