# Cheroke Admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2014 Alvaro Lopez Ortega
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#

import CTK
from Rule import RulePlugin
from util import *

URL_APPLY = '/plugin/bind/apply'

def commit():
    # POST info
    key      = CTK.post.pop ('key', None)
    vsrv_num = CTK.post.pop ('vsrv_num', None)
    new_bind = CTK.post.pop ('tmp!bind', None)

    # New entry
    if new_bind:
        next_rule, next_pre = cfg_vsrv_rule_get_next ('vserver!%s'%(vsrv_num))
        next_bind = CTK.cfg.get_next_entry_prefix ('%s!match!bind'%(next_pre))

        CTK.cfg['%s!match'%(next_pre)] = 'bind'
        CTK.cfg[next_bind]             = new_bind

        return {'ret': 'ok', 'redirect': '/vserver/%s/rule/%s' %(vsrv_num, next_rule)}

    # Modifications
    return CTK.cfg_apply_post()


class Plugin_bind (RulePlugin):
    def __init__ (self, key, **kwargs):
        RulePlugin.__init__ (self, key)
        self.vsrv_num = kwargs.pop('vsrv_num', '')

        if key.startswith('tmp'):
            return self.GUI_new()

        return self.GUI_mod()

    def _render_port_name (self, num):
        port = CTK.cfg.get_val ("server!bind!%s!port"%(num))
        inte = CTK.cfg.get_val ("server!bind!%s!interface"%(num))

        if inte:
            return (num, _("IP (Port):") + " %s (%s)"%(inte, port))
        return (num, _("Port:") + " %s"%(port))

    def GUI_new (self):
        # Build the port list
        ports = [('', _("Choose"))]
        for b in CTK.cfg.keys("server!bind"):
            tmp = self._render_port_name (b)
            ports.append(tmp)

        # Table
        table = CTK.PropsTable()
        table.Add (_('Incoming IP/Port'), CTK.ComboCfg('%s!bind'%(self.key), ports, {'class': 'noauto'}), '')

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('key', self.key)
        submit += CTK.Hidden ('vsrv_num', self.vsrv_num)
        submit += table
        self += submit

    def GUI_mod (self):
        pre = '%s!bind'%(self.key)
        tmp = CTK.cfg.keys (pre)
        tmp.sort(lambda x,y: cmp(int(x), int(y)))

        if tmp:
            table = CTK.Table({'id': 'rule-table-bind'})
            table.set_header(1)
            table += [CTK.RawHTML(x) for x in (_('Port'), _('Bind to'), _('TLS'), '')]
            for b in tmp:
                server_bind = CTK.cfg.get_val('%s!%s'%(pre, b))
                port = CTK.cfg.get_val ("server!bind!%s!port"%(server_bind))
                bind = CTK.cfg.get_val ("server!bind!%s!interface"%(server_bind), '')
                tls_ = CTK.cfg.get_val ("server!bind!%s!tls"%(server_bind), False)
                tls  = [_("No"), _("Yes")][int(tls_)]

                delete = None
                if len(tmp) > 1:
                    delete = CTK.ImageStock('del')
                    delete.bind('click', CTK.JS.Ajax (URL_APPLY,
                                                      data     = {'%s!%s'%(pre, b): ''},
                                                      complete = "$('#%s').trigger('submit_success');"%(self.id)))

                table += [CTK.RawHTML(port), CTK.RawHTML(bind), CTK.RawHTML(tls), delete]

            self += CTK.RawHTML ('<h3>%s</h3>' % (_('Selected Ports')))
            self += CTK.Indenter(table)

        # Don't show port already being listened to
        left = CTK.cfg.keys("server!bind")
        rule_used = []
        for b in CTK.cfg.keys (pre):
            port_num = CTK.cfg.get_val('%s!%s'%(pre,b))
            if port_num in left:
                left.remove(port_num)

        if left:
            ports = [('', _("Choose"))]
            for b in left:
                tmp = self._render_port_name (b)
                ports.append (tmp)

            next_bind = CTK.cfg.get_next_entry_prefix ('%s!bind'%(self.key))

            table = CTK.PropsTable()
            table.Add (_('Incoming IP/Port'), CTK.ComboCfg(next_bind, ports), '')

            self += CTK.RawHTML ('<h3>%s</h3>' % (_('Assign new IP/Port')))
            submit = CTK.Submitter (URL_APPLY)
            submit += table
            self += CTK.Indenter(submit)


    def GetName (self):
        tmp = []
        for b in CTK.cfg.keys('%s!bind'%(self.key)):
            real_n = CTK.cfg.get_val('%s!bind!%s'%(self.key, b))
            port = CTK.cfg.get_val('server!bind!%s!port'%(real_n))
            tls  = CTK.cfg.get_val('server!bind!%s!tls'%(real_n), False)
            tmp.append((port, bool(int(tls))))

        def render_entry (e):
            port, tls = e
            txt = port
            if tls:
                txt += ' (TLS)'
            return txt

        tmp.sort (lambda x,y: cmp(int(x[0]),int(y[0])))

        if len(tmp) == 0:
            return _("Port")
        if len(tmp) == 1:
            return "%s %s" %(_("Port"), render_entry(tmp[0]))

        txt = _("Ports")
        txt += " %s" %(", ".join([render_entry(x) for x in tmp[:-1]]))
        txt += " or %s" %(render_entry(tmp[-1]))
        return txt

CTK.publish ("^%s"%(URL_APPLY), commit, method="POST")
