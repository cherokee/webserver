from Form import *
from Table import *
from Module import *

from consts import *

class ModuleRoundRobin (Module, FormHelper):
    def __init__ (self, cfg, prefix):
        Module.__init__ (self, 'round_robin', cfg, prefix)
        FormHelper.__init__ (self, 'round_robin', cfg)

    def _op_render (self):
        txt = ''

        try:
            cfg = self._cfg[self._prefix]
            hosts = filter (lambda x: x != 'type', cfg)
        except:
            hosts = []

        # Render tables: as Hosts
        t1 = Table(2,1)
        t1 += ('Host', '')
        for host in hosts:
            pre = '%s!%s' % (self._prefix, host)
            e_host = self.InstanceEntry('%s!host'%(pre), 'text')
            t1 += (e_host, SUBMIT_DEL)

        en1 = self.InstanceEntry('new_host', 'text')
        t1 += (en1, SUBMIT_ADD)

        # Render tables: as Interpreters
        t2_txt = ''
        for host in hosts:
            pre = '%s!%s' % (self._prefix, host)
            e_host = self.InstanceEntry('%s!host'%(pre), 'text')
            e_inte = self.InstanceEntry('%s!interpreter'%(pre), 'text')
            e_envs = self._render_envs('%s!env'%(pre))

            t2 = Table(2)
            t2 += ('Host', e_host)
            t2 += ('Interpreter', e_inte)
            t2 += ('Environment', e_envs)
            t2_txt += str(t2)

        t2 = Table(3)
        e_host = self.InstanceEntry('new_host', 'text')
        e_inte = self.InstanceEntry('new_interpreter', 'text')
        t2 += (e_host, e_inte, SUBMIT_ADD)

        # General selector
        props = {}
        props ['host']        = str(t1)
        props ['interpreter'] = t2_txt

        table = Table(2)
        e = self.AddTableOptions_w_Properties (table, "Information sources", 
                                               "%s!type"%(self._prefix), BALANCER_TYPES, props)
        txt += str(table) + e

        return txt

    def __find_name (self):
        i = 1
        while True:
            key = "%s!%d" % (self._prefix, i)
            tmp = self._cfg[key]
            if not tmp: 
                return str(i)
            i += 1

    def _render_envs (self, cfg_key):
        """
        vserver!default!extensions!php!handler!balancer!entry1!env!PHP_FCGI_MAX_REQUESTS = 100
        vserver!default!extensions!php!handler!balancer!entry1!env!PHP_FCGI_CHILDREN = 2
        """
        txt = ''
        cfg = self._cfg[cfg_key]

        # Current environment
        if cfg and cfg.has_child():
            table = Table(3)
            for env in cfg:
                cfg_key_env = "%s!%s" % (cfg_key, env)

                js = "post_del_key('/%s/update', '%s');" % (self._id, cfg_key)
                print "!!!JS", js
                button = self.InstanceButton ('Del', onClick=js)
                self.AddTableEntry (table, env, cfg_key_env, (button,))
            txt += str(table)

        # Add new environment variable
        en_env = self.InstanceEntry('balancer_new_env', 'text')
        en_val = self.InstanceEntry('balancer_new_env_val', 'text')
        js     = "" # "post_del_key('/icons/update', '%s');" % (cfg_key)
        button = self.InstanceButton ('Add', onClick=js)

        table = Table(3,1)
        table += ('Variable', 'Value', '')
        table += (en_env, en_val, button)
        txt += str(table)

        return txt

    def _op_apply_changes (self, uri, post):
        # Add new 'Host' or 'Interpreter'
        if 'new_host' in post or \
           'new_interpreter' in post:
            num = self.__find_name()

        if post['new_host']:
            key = "%s!%s!host" % (self._prefix, num)
            self._cfg[key] = post['new_host'][0]
            del(post['new_host'])

        if post['new_interpreter']:
            key = "%s!%s!interpreter" % (self._prefix, num)
            self._cfg[key] = post['new_interpreter'][0]
            del(post['new_interpreter'])

        # Everything else
        self.ApplyChanges ([], post)

