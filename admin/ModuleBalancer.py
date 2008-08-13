from Form import *
from Table import *
from Module import *

from consts import *

NOTE_SOURCE   = 'The source can be either a local interpreter or a remote host acting as an application server.'
NOTE_BALANCER = 'Allow to select how the connections will be dispatched.'

class ModuleBalancerGeneric (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url, name):
        FormHelper.__init__ (self, name, cfg)
        Module.__init__ (self, name, cfg, prefix, submit_url)

        # Set the initial values
        self.default_type      = 'interpreter'
        self.allow_type_change = True
        self._type             = None

    def _get_type (self, post=None):
        # Post
        if post:
            new_type = post.get_val('%s!type'%(self._prefix))
            if new_type:
                return new_type

        # Config
        t = self._cfg.get_val ('%s!type'%(self._prefix))
        if not t: 
            return self.default_type
        return t

    def _op_render (self):
        txt = ''

        # Get the host/interpreter list
        try:
            cfg = self._cfg[self._prefix]
            hosts = filter (lambda x: x != 'type', cfg)
        except:
            hosts = []

        # Read the type
        if not self._type:
            self._type = self._get_type()

        # Options
        if self.allow_type_change:
            table = TableProps()
            self.AddPropOptions_Reload (table, "Information sources", 
                                        "%s!type" % (self._prefix), 
                                        BALANCER_TYPES, NOTE_SOURCE)
            txt += str(table)

        if self._type == 'host':
            # Host list
            if hosts:
                txt += "<h3>Hosts list</h3>"
                t1 = Table(2,1)
                t1 += ('Host', '')
                for host in hosts:
                    pre = '%s!%s' % (self._prefix, host)
                    e_host = self.InstanceEntry('%s!host'%(pre), 'text')
                    js = "post_del_key('/ajax/update', '%s');" % (pre)
                    link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                    t1 += (e_host, link_del)
                txt += self.Indent(t1)

            # New host
            txt += "<h3>Add new host</h3>"

            t1  = Table(2,1)
            t1 += ('New host', '')
            en1 = self.InstanceEntry('new_host', 'text')
            t1 += (en1, SUBMIT_ADD)
            txt += self.Indent(t1)

        elif self._type == 'interpreter':
            # Interpreter list
            if hosts:
                txt += "<h3>Hosts list</h3>"
                for host in hosts:
                    pre = '%s!%s' % (self._prefix, host)
                    e_host = self.InstanceEntry('%s!host'%(pre), 'text')
                    e_inte = self.InstanceEntry('%s!interpreter'%(pre), 'text')
                    e_envs = self._render_envs('%s!env'%(pre))

                    js = "post_del_key('/ajax/update', '%s');" % (pre)
                    link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)

                    t2 = Table(3, title_left=1)
                    t2 += ('Host', e_host, link_del)
                    t2 += ('Interpreter', e_inte)
                    t2 += ('Environment', e_envs)
                    txt += self.Indent(t2)
                    txt += "<hr />"

                if txt.endswith("<hr />"):
                    txt = txt[:-6]

            txt += "<h3>Add new host</h3>"

            # New Interpreter
            t2 = Table(3,1)
            t2 += ('Host', 'Interpreter', '')
            e_host = self.InstanceEntry('new_host', 'text', size=25)
            e_inte = self.InstanceEntry('new_interpreter', 'text', size=25)
            t2 += (e_host, e_inte, SUBMIT_ADD)
            
            txt += self.Indent(t2)
            
        else:
            txt = 'UNKNOWN type: ' + str(self._type)

        return txt

    def _render_envs (self, cfg_key):
        txt = ''
        cfg = self._cfg[cfg_key]

        # Current environment
        if cfg and cfg.has_child():
            table = Table(4)
            for env in cfg:
                host        = cfg_key.split('!')[1]
                cfg_key_env = "%s!%s" % (cfg_key, env)

                js = "post_del_key('/ajax/update', '%s');" % (cfg_key_env)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)
                table += (env, '=', self._cfg[cfg_key_env].value, link_del)
            txt += str(table)

        # Add new environment variable
        en_env = self.InstanceEntry('balancer_new_env',     'text', size=20)
        en_val = self.InstanceEntry('balancer_new_env_val', 'text', size=20)
        hidden = self.HiddenInput  ('balancer_new_env_key', cfg_key)

        table = Table(3,1)
        table += ('Variable', 'Value', '')
        table += (en_env, en_val, SUBMIT_ADD)
        txt += str(table) + hidden

        return txt

    def __find_name (self):
        i = 1
        while True:
            key = "%s!%d" % (self._prefix, i)
            tmp = self._cfg[key]
            if not tmp: 
                return str(i)
            i += 1

    def _op_apply_changes (self, uri, post):
        # Type
        if not self._type:
            self._type = self._get_type (post)
        self._cfg['%s!type'%(self._prefix)] = self._type

        # Addind new host/interpreter
        new_host        = post.pop('new_host')
        new_interpreter = post.pop('new_interpreter')

        if new_host or new_interpreter:
            num = self.__find_name()

            if new_host:
                key = "%s!%s!host" % (self._prefix, num)
                self._cfg[key] = new_host
        
            if new_interpreter and \
               self._type == 'interpreter':
                key = "%s!%s!interpreter" % (self._prefix, num)
                self._cfg[key] = new_interpreter
        
        # New environment variable
        if self._type == 'interpreter':
            env = post.pop('balancer_new_env')
            val = post.pop('balancer_new_env_val')
            key = post.pop('balancer_new_env_key')
            
            if env and val and key:
                self._cfg["%s!%s"%(key, env)] = val

        # Everything else
        self.ApplyChanges ([], post)
