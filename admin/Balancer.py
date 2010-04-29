# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2010 Alvaro Lopez Ortega
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

URL_APPLY = '/plugin/balancer/apply'

NOTE_BALANCER      = N_('Specifies the policy used to dispatch the connections.')
NOTE_ALL_SOURCES   = N_('It is already balancing among all the configured <a href="%s">Information Sources</a>.')
NO_GENERAL_SOURCES = N_('There are no Information Sources configured. Please proceed to configure an <a href="/source">Information Source</a>.')
NO_SOURCE_WARNING  = N_('A load balancer must be configured to use at least one information source.')


def commit():
    new_bal = CTK.post.pop ('tmp!new_balancer_node')
    key     = CTK.post.pop ('key')

    # New
    if new_bal:
        next = CTK.cfg.get_next_entry_prefix ('%s!source'%(key))
        CTK.cfg[next] = new_bal

    # Modification
    return CTK.cfg_apply_post()


class PluginBalancer (CTK.Plugin):
    def __init__ (self, key, **kwargs):
        CTK.Plugin.__init__ (self, key)

    class ContentSources (CTK.Container):
        def __init__ (self, refresh, key):
            CTK.Container.__init__ (self)

            general_sources  = CTK.cfg.keys('source')
            balancer_sources = CTK.cfg.keys('%s!source'%(key))

            if not balancer_sources:
                self += CTK.Notice ('warning', CTK.RawHTML (_(NO_SOURCE_WARNING)))
            else:
                table = CTK.Table({'id': 'balancer-table'})
                table.set_header(1)
                table += [CTK.RawHTML(x) for x in (_('Nick'), _('Host'), '')]
                for sb in balancer_sources:
                    sg   = CTK.cfg.get_val ('%s!source!%s'%(key, sb))
                    nick = CTK.cfg.get_val ('source!%s!nick'%(sg))
                    host = CTK.cfg.get_val ('source!%s!host'%(sg))
                    link = CTK.Link ("/source/%s"%(sg), CTK.RawHTML (nick))

                    remove = None
                    if len(balancer_sources) >= 2:
                        remove = CTK.ImageStock('del')
                        remove.bind('click', CTK.JS.Ajax (URL_APPLY,
                                                          data     = {'%s!source!%s'%(key, sb): ''},
                                                          complete = refresh.JS_to_refresh()))

                    table += [link, CTK.RawHTML(host), remove]

                submit = CTK.Submitter (URL_APPLY)
                submit += table
                self += submit

    def AddCommon (self):
        general_sources  = CTK.cfg.keys('source')
        balancer_sources = CTK.cfg.keys('%s!source'%(self.key))

        # These are the sources that have not been added yet
        general_left = general_sources[:]
        for sb in balancer_sources:
            sg = CTK.cfg.get_val('%s!source!%s'%(self.key, sb))
            if sg in general_left:
                while True:
                    try: general_left.remove(sg)
                    except: break

        if not balancer_sources and not general_left:
            self += CTK.Notice ('warning', CTK.RawHTML (_(NO_GENERAL_SOURCES)))
            return

        # Configured sources List
        refresh = CTK.Refreshable ({'id': 'balancer'})
        refresh.register (lambda: self.ContentSources(refresh, self.key).Render())

        self += CTK.RawHTML ("<h2>%s</h2>" %(_('Information Sources')))
        self += CTK.Indenter (refresh)

        # Assign new sources
        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Assign Information Sources')))

        if not general_left:
            self += CTK.Indenter (CTK.Notice('information', CTK.RawHTML (_(NOTE_ALL_SOURCES)%("/source"))))
        else:
            options = [('', _('Choose…'))]
            for s in general_left:
                nick = CTK.cfg.get_val('source!%s!nick'%(s))
                options.append((s,nick))

            table = CTK.PropsTable()
            table.Add (_("Application Server"), CTK.ComboCfg("tmp!new_balancer_node", options), '')

            submit = CTK.Submitter (URL_APPLY)
            submit += CTK.Hidden ('key', self.key)
            submit += table
            submit.bind ('submit_success', refresh.JS_to_refresh())

            self += CTK.Indenter (submit)


CTK.publish ('^%s'%(URL_APPLY), commit, method="POST")
