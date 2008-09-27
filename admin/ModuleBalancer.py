from Form import *
from Table import *
from Module import *

from consts import *

NOTE_BALANCER      = 'Allow to select how the connections will be dispatched.'
NO_GENERAL_SOURCES = 'There are no Information Sources configured. Please proceed to configure an <a href="/source">Info Source</a>.'
NO_SOURCE_WARNING  = 'A load balancer must be configured to use at least one data source.'

class ModuleBalancerGeneric (Module, FormHelper):
    def __init__ (self, cfg, prefix, submit_url, name):
        Module.__init__ (self, name, cfg, prefix, submit_url)
        FormHelper.__init__ (self, name, cfg)

    def _op_render (self):
        txt = ''
        general_sources  = self._cfg.keys('source')
        balancer_sources = self._cfg.keys('%s!source'%(self._prefix))

        # These are the sources that have not been added yet
        general_left = general_sources[:]
        for sb in balancer_sources:
            sg = self._cfg.get_val('%s!source!%s'%(self._prefix, sb))
            if sg in general_left:
                while True:
                    try: general_left.remove(sg)
                    except: break

        if not balancer_sources and not general_left:
            txt += self.Dialog (NO_GENERAL_SOURCES, type_='warning')
            return txt

        if not balancer_sources:
            txt += self.Dialog(NO_SOURCE_WARNING, type_='warning')
        else:
            txt += '<h2>Info Sources</h2>'
            table = Table(3,1, style='width="100%"')
            table += ('Nick', 'Host', '')
            for sb in balancer_sources:
                sg = self._cfg.get_val ('%s!source!%s'%(self._prefix, sb))

                nick  = self._cfg.get_val('source!%s!nick'%(sg))
                host  = self._cfg.get_val('source!%s!host'%(sg))
                link  = '<a href="/source/%s">%s</a>' % (sb, nick)

                js = "post_del_key('/ajax/update', '%s!source!%s');"%(self._prefix, sb)
                link_del = self.InstanceImage ("bin.png", "Delete", border="0", onClick=js)

                table += (link, host, link_del)
            txt += str(table)

        txt += '<h2>Assign Info Sources</h2>'
        if not general_left:
            txt += 'It is already balancing among all the configured ' + \
                   '<a href="/source">information sources</a>.'
        else:
            options = [('', 'Choose..')]
            for s in general_left:
                nick = self._cfg.get_val('source!%s!nick'%(s))
                options.append((s,nick))

            table = TableProps()
            self.AddPropOptions_Reload (table, "Application Server",
                                        "new_balancer_node", options, "")
            txt += str(table)

        return txt

    def _op_apply_changes (self, uri, post):
        new_balancer_node = post.pop('new_balancer_node')
        if new_balancer_node:
            tmp = [int(x) for x in self._cfg.keys('%s!source'%(self._prefix))]
            tmp.sort()

            if tmp:
                new_source = str(tmp[-1]+1)
            else:
                new_source = 1

            self._cfg['%s!source!%s'%(self._prefix, new_source)] = new_balancer_node
