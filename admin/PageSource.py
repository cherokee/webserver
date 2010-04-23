# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#      Taher Shihadeh <taher@unixwars.com>
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

import re
import CTK
import Page
import Cherokee
import SelectionPanel
import validations
import Rule

from consts import *
from CTK.util import find_copy_name
from CTK.Submitter import HEADER as Submit_HEADER
from CTK.TextField import HEADER as TextField_HEADER


URL_BASE    = '/source'
URL_CONTENT = '/source/content'
URL_APPLY   = '/source/apply'

NOTE_SOURCE        = N_('The source can be either a local interpreter or a remote host acting as an information source.')
NOTE_NICK          = N_('Source nick. It will be referenced by this name in the rest of the server.')
NOTE_TYPE          = N_('It allows to choose whether it runs the local host or a remote server.')
NOTE_HOST          = N_('Where the information source can be accessed. The host:port pair, or the Unix socket path.')
NOTE_INTERPRETER   = N_('Command to spawn a new source in case it were not accessible.')
NOTE_TIMEOUT       = N_('How long should the server wait when spawning an interpreter before giving up (in seconds). Default: 3.')
NOTE_USAGE         = N_('Sources currently in use. Note that the last source of any rule cannot be deleted until the rule has been manually edited.')
NOTE_USER          = N_('Execute the interpreter under a different user. Default: Same UID as the server.')
NOTE_GROUP         = N_('Execute the interpreter under a different group. Default: Default GID of the new process UID.')
NOTE_ENV_INHERIT   = N_('Whether the new child process should inherit the environment variables from the server process. Default: yes.')
NOTE_DELETE_DIALOG = N_('You are about to delete an Information Source. Are you sure you want to proceed?')
NOTE_NO_ENTRIES    = N_('The Information Source list is currently empty.')
NOTE_FORBID_1      = N_('This is the last Information Source in use by a rule. Deleting it would break the configuration.')
NOTE_FORBID_2      = N_('First edit the offending rule(s)')

VALIDATIONS = [
    ('source!.+?!host',        validations.is_information_source),
    ('source!.+?!timeout',     validations.is_positive_int),
    ('tmp!new_host',           validations.is_safe_information_source),
    ("source_clone_trg",       validations.is_safe_id),
]

HELPS = [('config_info_sources', N_("Information Sources"))]


JS_ACTIVATE_LAST = """
$('.selection-panel:first').data('selectionpanel').select_last();
"""

JS_CLONE = """
  var panel = $('.selection-panel:first').data('selectionpanel').get_selected();
  var url   = panel.find('.row_content').attr('url');
  $.ajax ({type: 'GET', async: false, url: url+'/clone', success: function(data) {
      // A transaction took place
      $('.panel-buttons').trigger ('submit_success');
  }});
"""

JS_PARTICULAR = """
  var source = window.location.pathname.match (/^\/source\/(\d+)/)[1];
  $.cookie ('%(cookie_name)s', source, { path: '/source' });
  window.location.replace ('%(url_base)s');
"""


def commit_clone():
    num = re.findall(r'^%s/([\d]+)/clone$'%(URL_CONTENT), CTK.request.url)[0]
    next = CTK.cfg.get_next_entry_prefix ('source')

    orig  = CTK.cfg.get_val ('source!%s!nick'%(num))
    names = [CTK.cfg.get_val('source!%s!nick'%(x)) for x in CTK.cfg.keys('source')]
    new_nick = find_copy_name (orig, names)

    CTK.cfg.clone ('source!%s'%(num), next)
    CTK.cfg['%s!nick' %(next)] = new_nick
    return CTK.cfg_reply_ajax_ok()


def commit():
    new_nick = CTK.post.pop('tmp!new_nick')
    new_host = CTK.post.pop('tmp!new_host')

    # New
    if new_nick and new_host:
        next = CTK.cfg.get_next_entry_prefix ('source')
        CTK.cfg['%s!nick'%(next)] = new_nick
        CTK.cfg['%s!host'%(next)] = new_host
        CTK.cfg['%s!type'%(next)] = 'host'
        return CTK.cfg_reply_ajax_ok()

    # Modification
    return CTK.cfg_apply_post()


def _get_used_sources ():
    """List of every rule using any info source"""
    used_sources = {}
    target ='!balancer!source!'
    for entry in CTK.cfg.serialize().split():
        if target in entry:
            source = CTK.cfg.get_val(entry)
            if not used_sources.has_key(source):
                used_sources[source] = []
            used_sources[source].append(entry)
    return used_sources


def _get_rule_sources ():
    # List of sources used by each rule
    rule_sources = {}
    used_sources = _get_used_sources()
    for src in used_sources:
        for r in used_sources[src]:
            rule = r.split('!handler!balancer!')[0]
            if not rule_sources.has_key(rule):
                rule_sources[rule] = []
            rule_sources[rule].append(src)
    return rule_sources


def _get_protected_sources ():
    # List of sources to protect against deletion"""
    rule_sources = _get_rule_sources()
    protected_sources = []
    for s in rule_sources:
        if len(rule_sources[s])==1:
            protected_sources.append(rule_sources[s][0])
    return protected_sources


class Render_Source:
    class Source_Usage (CTK.Container):
        def __init__ (self, rules):
            CTK.Container.__init__ (self)

            table = CTK.Table({'id': 'source-usage'})
            self += table
            table.set_header(1)
            table += [CTK.RawHTML(x) for x in (_('Virtual Server'), _('Rule'))]

            for rule in rules:
                vsrv_num  = rule.split('!')[1]
                vsrv_name = CTK.cfg.get_val ("vserver!%s!nick" %(vsrv_num), _("Unknown"))
                r = Rule.Rule('%s!match' %(rule))
                rule_name = r.GetName()
                table += [CTK.RawHTML(x) for x in (vsrv_name, rule_name)]


    def __call__ (self):
        # /source/empty
        if CTK.request.url.endswith('/empty'):
            notice = CTK.Notice ('information', CTK.RawHTML (NOTE_NO_ENTRIES))
            return notice.Render().toJSON()

        # /source/content/\d+
        num = re.findall(r'^%s/([\d]+)$'%(URL_CONTENT), CTK.request.url)[0]

        tipe = CTK.cfg.get_val('source!%s!type'%(num))
        nick = CTK.cfg.get_val('source!%s!nick'%(num))

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>Source: %s</h2>'%(nick))

        workarea = CTK.Box ({'id': 'source-workarea'})

        table = CTK.PropsTable()
        table.Add (_('Type'),       CTK.ComboCfg ('source!%s!type'%(num), SOURCE_TYPES), _(NOTE_TYPE))
        table.Add (_('Nick'),       CTK.TextCfg ('source!%s!nick'%(num), False), _(NOTE_NICK))
        table.Add (_('Connection'), CTK.TextCfg ('source!%s!host'%(num), False), _(NOTE_HOST))
        if tipe == 'interpreter':
            table.Add (_('Interpreter'),         CTK.TextCfg ('source!%s!interpreter'%(num),   False), _(NOTE_INTERPRETER))
            table.Add (_('Spawning timeout'),    CTK.TextCfg ('source!%s!timeout'%(num),       True),  _(NOTE_TIMEOUT))
            table.Add (_('Execute as User'),     CTK.TextCfg ('source!%s!user'%(num),          True),  _(NOTE_USER))
            table.Add (_('Execute as Group'),    CTK.TextCfg ('source!%s!group'%(num),         True),  _(NOTE_GROUP))
            table.Add (_('Inherit Environment'), CTK.TextCfg ('source!%s!env_inherited'%(num), False), _(NOTE_ENV_INHERIT))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('source', num)
        submit += table
        workarea += submit

        sources = _get_rule_sources ()
        rules   = [key for key,val in sources.items() if str(num) in val]

        if rules:
            workarea += self.Source_Usage (rules)
 
        cont += workarea

        render = cont.Render()
        return render.toJSON()


class CloneSource (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)
        self += CTK.RawHTML ('Information Source Cloning dialog.')


class AddSource (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        table = CTK.PropsTable()
        table.Add (_('Nick'),       CTK.TextCfg ('tmp!new_nick', False, {'class': 'noauto'}), _(NOTE_NICK))
        table.Add (_('Connection'), CTK.TextCfg ('tmp!new_host', False, {'class': 'noauto'}), _(NOTE_HOST))

        submit = CTK.Submitter (URL_APPLY)
        submit += table
        self += submit

class Render_Particular:
    def __call__ (self):
        headers = SelectionPanel.HEADER
        page    = CTK.Page(headers=headers)

        props = {'url_base':    URL_BASE,
                 'cookie_name': SelectionPanel.COOKIE_NAME_DEFAULT}

        page += CTK.RawHTML (js=JS_PARTICULAR %(props))
        return page.Render()

class Render:
    class PanelList (CTK.Container):
        def __init__ (self, refresh, right_box):
            CTK.Container.__init__ (self)

            # Helper
            entry = lambda klass, key: CTK.Box ({'class': klass}, CTK.RawHTML (CTK.cfg.get_val(key, '')))

            # Build the panel list
            panel = SelectionPanel.SelectionPanel (None, right_box.id, "/source", '%s/empty'%(URL_CONTENT), draggable=False, container='source_panel')
            self += panel

            sources = CTK.cfg.keys('source')
            sources.sort (lambda x,y: cmp (int(x), int(y)))

            self.protected_sources = _get_protected_sources ()
            self.rule_sources      = _get_rule_sources ()

            for k in sources:
                tipe = CTK.cfg.get_val('source!%s!type'%(k))
                dialog = self._get_dialog (k, refresh)
                self += dialog

                remove = CTK.ImageStock('del')
                remove.bind ('click', dialog.JS_to_show() + "return false;")

                group = CTK.Box ({'class': 'sel-actions'}, [remove])

                if tipe == 'host':
                    panel.Add (k, '%s/%s'%(URL_CONTENT, k), [group,
                                                             entry('nick',  'source!%s!nick'%(k)),
                                                             entry('type',  'source!%s!type'%(k)),
                                                             entry('host',  'source!%s!host'%(k))])
                elif tipe == 'interpreter':
                    panel.Add (k, '%s/%s'%(URL_CONTENT, k), [group,
                                                             entry('nick',  'source!%s!nick'%(k)),
                                                             entry('type',  'source!%s!type'%(k)),
                                                             entry('host',  'source!%s!host'%(k)),
                                                             entry('inter', 'source!%s!interpreter'%(k))])

        def _get_dialog (self, k, refresh):
            if k in self.protected_sources:
                rules = []
                for rule, sources in self.rule_sources.items():
                    if str(k) in sources:
                        rules.append(rule)

                links = []
                for rule in rules:
                    r = Rule.Rule('%s!match' %(rule))
                    rule_name = r.GetName()
                    rule_link = rule.replace('!','/')
                    links.append(CTK.consts.LINK_HREF%(rule_link, rule_name))

                dialog  = CTK.Dialog ({'title': _('Deletion is forbidden'), 'width': 480})
                dialog += CTK.RawHTML (NOTE_FORBID_1)
                dialog += CTK.RawHTML (_('<p>%s: %s</p>')%(NOTE_FORBID_2, ', '.join(links)))
                dialog.AddButton (_('Close'), "close")

            else:
                dialog = CTK.Dialog ({'title': _('Do you really want to remove it?'), 'width': 480})
                dialog.AddButton (_('Remove'), CTK.JS.Ajax (URL_APPLY, async=False,
                                                            data    = {'source!%s'%(k):''},
                                                            success = dialog.JS_to_close() + \
                                                                      refresh.JS_to_refresh()))
                dialog.AddButton (_('Cancel'), "close")
                dialog += CTK.RawHTML (NOTE_DELETE_DIALOG)

            return dialog



    class PanelButtons (CTK.Box):
        def __init__ (self):
            CTK.Box.__init__ (self, {'class': 'panel-buttons'})

            # Add New
            dialog = CTK.Dialog ({'title': _('Add New Information Source'), 'width': 380})
            dialog.AddButton (_('Add'), dialog.JS_to_trigger('submit'))
            dialog.AddButton (_('Cancel'), "close")
            dialog += AddSource()

            button = CTK.Button(_('New…'), {'id': 'source-new-button', 'class': 'panel-button', 'title': _('Add New Information Source')})
            button.bind ('click', dialog.JS_to_show())
            dialog.bind ('submit_success', dialog.JS_to_close())
            dialog.bind ('submit_success', self.JS_to_trigger('submit_success'));

            self += button
            self += dialog

            # Clone
            dialog = CTK.Dialog ({'title': _('Clone Information Source'), 'width': 480})
            dialog.AddButton (_('Clone'), JS_CLONE + dialog.JS_to_close())
            dialog.AddButton (_('Cancel'), "close")
            dialog += CloneSource()

            button = CTK.Button(_('Clone…'), {'id': 'source-clone-button', 'class': 'panel-button', 'title': _('Clone Selected Information Source')})
            button.bind ('click', dialog.JS_to_show())

            self += dialog
            self += button

    def __call__ (self):
        # Content
        left  = CTK.Box({'class': 'panel'})
        left += CTK.RawHTML('<h2>%s</h2>'%(_('Information Sources')))

        # Sources List
        refresh = CTK.Refreshable ({'id': 'source_panel'})
        refresh.register (lambda: self.PanelList(refresh, right).Render())

        # Refresh on 'New' or 'Clone'
        buttons = self.PanelButtons()
        buttons.bind ('submit_success', refresh.JS_to_refresh (on_success=JS_ACTIVATE_LAST))
        left += buttons

        left += CTK.Box({'class': 'filterbox'}, CTK.TextField({'class':'filter', 'optional_string': _('Sources Filtering'), 'optional': True}))
        right = CTK.Box({'class': 'source_content'})
        left += refresh

        # Refresh the list whenever the content change
        right.bind ('submit_success', refresh.JS_to_refresh());

        # Build the page
        headers = Submit_HEADER + TextField_HEADER
        page = Page.Base (_("Information Sources"), body_id='source', helps=HELPS, headers=headers)
        page += left
        page += right

        return page.Render()

CTK.publish ('^%s$'              %(URL_BASE), Render)
CTK.publish ('^%s/\d+$'          %(URL_BASE), Render_Particular)
CTK.publish ('^%s/content/[\d]+$'%(URL_BASE), Render_Source)
CTK.publish ('^%s/content/empty$'%(URL_BASE), Render_Source)
CTK.publish ('^%s/content/[\d]+/clone$'%(URL_BASE), commit_clone)
CTK.publish ('^%s$'              %(URL_APPLY), commit, validation=VALIDATIONS, method="POST")
