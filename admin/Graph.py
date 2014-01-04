# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#      Taher Shihadeh <taher@unixwars.com>
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
import Cherokee
import validations

from util import *
from consts import *
from configured import *


URL_APPLY       = '/graph/apply'

GRAPH_VSERVER   = '/graphs/%(prefix)s_%(type)s_%(vserver)s_%(interval)s.png'
GRAPH_SERVER    = '/graphs/%(prefix)s_%(type)s_%(interval)s.png'

GRAPH_TYPES     = [('traffic',  N_('Server Traffic')),
                   ('accepts',  N_('Connections / Requests')),
                   ('timeouts', N_('Connection Timeouts')),]

GRAPH_INTERVALS = [('1h', N_('1 Hour')),
                   ('6h', N_('6 Hours')),
                   ('1d', N_('1 Day')),
                   ('1w', N_('1 Week')),
                   ('1m', N_('1 Month'))]

URL_RRD          = '/general#Network-1'
RRD_NOTICE       = N_('You need to enable the <a href="%s">Information Collector</a> setting in order to render usage graphics.')
NOTE_COLLECTOR   = N_('Whether or not it should collect statistics about the traffic of this virtual server.')

UPDATE_JS = """
function updateGraph() {
    var timeout = $(window).data('graph-timeout');

    if ((timeout != null) &&
        (timeout != undefined))
    {
        clearTimeout (timeout);
        $(window).data('graph-timeout', null);
    }

    %s
}

var tmp = setTimeout (updateGraph, 60000);
$(window).data('graph-timeout', tmp);
""" #%(JS_to_refresh)

def apply ():
    graph_type = CTK.post.get_val('graph_type')
    if graph_type:
        CTK.cfg['tmp!graph_type'] = graph_type
        return CTK.cfg_reply_ajax_ok()

    # Modifications
    return CTK.cfg_apply_post()


class Graph (CTK.Box):
    def __init__ (self, refreshable, **kwargs):
        CTK.Box.__init__ (self)
        self.graph = {}
        self.graph['type'], self.graph['type_txt'] = GRAPH_TYPES[0]
        self.refresh = refreshable

    def build_graph (self):
        if CTK.cfg.get_val('server!collector') != 'rrd':
            notice  = CTK.Notice()
            notice += CTK.RawHTML(_(RRD_NOTICE)%(URL_RRD))
            self   += CTK.Indenter(notice)
            return False

        tabs = CTK.Tab ()
        for x in GRAPH_INTERVALS:
            self.graph['interval'] = x[0]
            props = {'src': self.template % self.graph,
                     'alt': '%s: %s' %(self.graph['type_txt'], x[1])}
            image = CTK.Image(props)
            tabs.Add (_(x[1]), image)
        self += tabs
        return True

    def Render (self):
        render     = CTK.Box.Render (self)
        render.js += UPDATE_JS % (self.refresh.JS_to_refresh())
        return render


class GraphVServer_Instancer (CTK.Container):
    class GraphVServer (Graph):
        def __init__ (self, refreshable, vsrv_num, **kwargs):
            Graph.__init__ (self, refreshable, **kwargs)
            self.template         = GRAPH_VSERVER
            self.graph['prefix']  = 'vserver'
            self.graph['vserver'] = CTK.cfg.get_val ("vserver!%s!nick" %(vsrv_num), _("Unknown"))
            self.graph['num']     = vsrv_num
            self.collector        = bool(int(CTK.cfg.get_val ('vserver!%s!collector!enabled'%(vsrv_num),'0')))
            self.build()

        def build (self):
            table = CTK.PropsTable()
            table.Add (_('Collect Statistics'),  CTK.CheckCfgText('vserver!%s!collector!enabled'%(self.graph['num']), self.collector, _('Enabled')), _(NOTE_COLLECTOR))

            submit  = CTK.Submitter (URL_APPLY)
            submit += table
            submit.bind('submit_success', self.refresh.JS_to_refresh())

            if self.collector:
                self.build_graph ()
                props = {'class': 'graph_props_bottom'}
            else:
                props = {'class': 'graph_props_top'}

            if CTK.cfg.get_val('server!collector') == 'rrd':
                self += CTK.Indenter (CTK.Box(props, submit))

    def __init__ (self, vserver):
        CTK.Container.__init__ (self)

        # Refresher
        refresh = CTK.Refreshable ({'id': 'grapharea'})
        refresh.register (lambda: self.GraphVServer(refresh, vserver).Render())
        self += refresh


class GraphServer_Instancer (CTK.Container):
    class GraphServer (Graph):
        def __init__ (self, refreshable, **kwargs):
            Graph.__init__ (self, refreshable, **kwargs)
            self.template        = GRAPH_SERVER
            self.graph['prefix'] = 'server'
            self.graph['type']   = CTK.cfg.get_val('tmp!graph_type', self.graph['type'])
            self.build()

        def build (self):
            rrd = self.build_graph ()
            if rrd:
                props   = {'class': 'graph_type', 'name': 'graph_type', 'selected': self.graph['type']}
                combo   = CTK.Combobox (props, trans_options(GRAPH_TYPES))
                submit  = CTK.Submitter (URL_APPLY)
                submit += combo
                submit.bind('submit_success', self.refresh.JS_to_refresh())

                for x in GRAPH_TYPES:
                    if x[0] == self.graph['type']:
                        self.graph['type_txt'] = x[1]

                self   += submit

    def __init__ (self):
        CTK.Container.__init__ (self)

        # Refresher
        refresh = CTK.Refreshable ({'id': 'grapharea'})
        refresh.register (lambda: self.GraphServer(refresh).Render())
        self += refresh

CTK.publish ('^%s'%(URL_APPLY), apply, method="POST")
