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

import re
import CTK
from CTK.consts import LINK_HREF
import Page
from CTK.i18n import _, N_
import SelectionPanel
import Rule

from util import *
from consts import *

from CTK.util import find_copy_name
from CTK.Submitter import HEADER as Submit_HEADER
from CTK.TextField import HEADER as TextField_HEADER

COOKIE = 'source_panel'

URL_BASE    = '/source'
URL_CONTENT = '/source/content'
URL_APPLY   = '/source/apply'

NOTE_SOURCE        = N_('The source can be either a local interpreter or a remote host acting as an information source.')
NOTE_NICK          = N_('Source nick. It will be referenced by this name in the rest of the server.')
NOTE_TYPE          = N_('It allows to choose whether it runs the local host or a remote server.')
NOTE_HOST          = N_('Where the information source can be accessed. The host:port pair, or the Unix socket path.')
NOTE_INTERPRETER1  = N_('Command to spawn a new source in case it were not running.')
NOTE_INTERPRETER2  = N_('REMEMBER: The interpreter MUST NOT run as a daemon or detach itself from its parent process.')
NOTE_TIMEOUT       = N_('How long should the server wait when spawning an interpreter before giving up (in seconds). Default: 3.')
NOTE_USAGE         = N_('Sources currently in use. Note that the last source of any rule cannot be deleted until the rule has been manually edited.')
NOTE_USER          = N_('Execute the interpreter under a different user. Default: Same UID as the server.')
NOTE_GROUP         = N_('Execute the interpreter under a different group. Default: Default GID of the new process UID.')
NOTE_CHROOT        = N_('Execute the interpreter under a different root. Default: no.')
NOTE_ENV_INHERIT   = N_('Whether the new child process should inherit the environment variables from the server process. Default: yes.')
NOTE_DELETE_DIALOG = N_('You are about to delete an Information Source. Are you sure you want to proceed?')
NOTE_NO_ENTRIES    = N_('The Information Source list is currently empty.')
NOTE_FORBID_1      = N_('This is the last Information Source in use by a rule. Deleting it would break the configuration.')
NOTE_FORBID_2      = N_('First edit the offending rule(s)')
NOTE_ADD_VARIABLE  = N_('Name of the environment variable')
NOTE_ADD_VALUE     = N_('Value of the environment variable')
NOTE_CLONE_DIALOG  = N_('The selected Information Source is about to be cloned.')

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
  var source = window.location.pathname.match (/^\\/source\\/(\\d+)/)[1];
  $.cookie ('%(cookie_name)s', source, { path: '/source' });
  window.location.replace ('%(url_base)s' + window.location.hash);
"""

def commit_clone():
    num = re.findall(rf'^{URL_CONTENT}/([\d]+)/clone$', CTK.request.url)[0]
    next = CTK.cfg.get_next_entry_prefix ('source')

    orig  = CTK.cfg.get_val (f'source!{num}!nick')
    names = [CTK.cfg.get_val(f'source!{x}!nick') for x in CTK.cfg.keys('source')]
    new_nick = find_copy_name (orig, names)

    CTK.cfg.clone (f'source!{num}', next)
    CTK.cfg[f'{next}!nick'] = new_nick
    return CTK.cfg_reply_ajax_ok()


def commit():
    new_nick = CTK.post.pop('tmp!new_nick')
    new_host = CTK.post.pop('tmp!new_host')

    # New source
    if new_nick and new_host:
        next = CTK.cfg.get_next_entry_prefix ('source')
        CTK.cfg[f'{next}!nick'] = new_nick
        CTK.cfg[f'{next}!host'] = new_host
        CTK.cfg[f'{next}!type'] = 'host'
        return CTK.cfg_reply_ajax_ok()

    # New variable
    new_variable = CTK.post.pop('tmp!new_variable')
    new_value    = CTK.post.pop('tmp!new_value')
    source_pre   = CTK.post.pop('tmp!source_pre')
    if new_variable and new_value:
        CTK.cfg[f'{source_pre}!{new_variable}'] = new_value
        return CTK.cfg_reply_ajax_ok()

    return CTK.cfg_apply_post()


def _all_sources_per_rule ():
    """Return list of {rule: [sources used by rule]}"""
    result = []
    vservers = CTK.cfg.keys('vserver')

    for vsrv in vservers:
        rules = CTK.cfg.keys(f'vserver!{vsrv}!rule')
        for rule_num in rules:
            rule    = f'vserver!{vsrv}!rule!{rule_num}'
            sources = _sources_per_rule (rule)
            if sources:
                result.append({rule: sources})

    return result


def _sources_per_rule (rule):
    """Return list of sources used by a given rule"""
    sources = []
    handler = CTK.cfg.get_val(f'{rule}!handler')

    if handler in HANDLERS_WITH_BALANCER:
        pre      = f'{rule}!handler!balancer!source'
        src_keys = CTK.cfg.keys(pre)
        for src_key in src_keys:
            sources.append (CTK.cfg.get_val(f'{pre}!{src_key}'))

    return list(set(sources))


def _rules_per_source (source):
    """Return list of complete rules using a given source number"""
    rules = []
    source_usage = _all_sources_per_rule ()

    for rule_dict in source_usage:
        rule_pre, sources = list(rule_dict.items())[0]
        if source in sources:
            pre = f'{rule_pre}!handler!balancer!source'
            balanced = CTK.cfg.keys (pre)
            for src in balanced:
                rule = f'{pre}!{src}'
                if CTK.cfg.get_val(rule) == source:
                    rules.append (rule)

    return list(set(rules))


def _protected_sources ():
    """Return list of sources that must be protected"""
    protect = []
    rules   = _all_sources_per_rule()
    for rule in rules:
        sources = list(rule.values())[0]
        if len(sources) == 1:
            protect.append(sources[0])
    return protect


class EnvironmentTable (CTK.Submitter):
    def __init__ (self, refreshable, src_num, **kwargs):
        CTK.Submitter.__init__ (self, URL_APPLY)

        variables = CTK.cfg.keys(f'source!{src_num}!env')
        if variables:
            table = CTK.Table({'id':'env_table'})
            table[(1,1)] = [CTK.RawHTML(x) for x in (_('Variable'), _('Value'), '')]
            table.set_header (row=True, num=1)
            self += CTK.RawHTML (f"<h2>{_('Environment Variables')}</h2>")
            self += table

            n = 2
            for v in variables:
                # Entries
                key   = f'source!{src_num}!env!{v}'
                value = CTK.TextCfg (key, True,  {'class': 'value'})
                delete = CTK.ImageStock('del')
                delete.bind('click', CTK.JS.Ajax (URL_APPLY,
                                                  data     = {key: ''},
                                                  complete = refreshable.JS_to_refresh()))

                table[(n,1)] = [CTK.RawHTML (v), value, delete]
                n += 1

class EnvironmentWidget (CTK.Container):
    def __init__ (self, src_num):
        CTK.Container.__init__ (self)

        # List ports
        self.refresh = CTK.Refreshable({'id': 'environment_table'})
        self.refresh.register (lambda: EnvironmentTable(self.refresh, src_num).Render())

        # Add new - dialog
        table = CTK.PropsTable()
        table.Add (_('Variable'), CTK.TextCfg ('tmp!new_variable', False, {'class':'noauto'}), _(NOTE_ADD_VARIABLE))
        table.Add (_('Value'),    CTK.TextCfg ('tmp!new_value',    False, {'class':'noauto'}), _(NOTE_ADD_VALUE))

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden ('tmp!source_pre',f'source!{src_num}!env')
        submit += table

        dialog = CTK.Dialog({'title': _('Add new Environment variable'), 'autoOpen': False, 'draggable': False, 'width': 530})
        dialog.AddButton (_("Cancel"), "close")
        dialog.AddButton (_("Add"),    submit.JS_to_submit())
        dialog += submit

        submit.bind ('submit_success', self.refresh.JS_to_refresh())
        submit.bind ('submit_success', dialog.JS_to_close())

        # Add new
        button = CTK.SubmitterButton (f"{_('Add new variable')}…")
        button.bind ('click', dialog.JS_to_show())
        button_s = CTK.Submitter (URL_APPLY)
        button_s += button

        # Integration
        self += self.refresh
        self += button_s
        self += dialog


class Render_Source:
    class Source_Usage (CTK.Container):
        def __init__ (self, rules):
            CTK.Container.__init__ (self)

            table = CTK.Table({'id': 'source-usage'})
            self += CTK.RawHTML (f"<h2>{_('Source Usage')}</h2>")
            self += table
            table.set_header(1)
            table += [CTK.RawHTML(x) for x in (_('Virtual Server'), _('Rule'))]

            for rule in rules:
                vsrv_num  = rule.split('!')[1]
                vsrv_name = CTK.cfg.get_val (f"vserver!{vsrv_num}!nick", _("Unknown"))
                r = Rule.Rule(f'{rule}!match')
                rule_name = r.GetName()
                vsrv_link = f'/vserver/{vsrv_num}'
                rule_link = rule.replace('!','/')
                table += [CTK.Link (vsrv_link, CTK.RawHTML(vsrv_name)),
                          CTK.Link (rule_link, CTK.RawHTML(rule_name))]


    def __call__ (self):
        # /source/empty
        if CTK.request.url.endswith('/empty'):
            notice = CTK.Notice ('information', CTK.RawHTML (_(NOTE_NO_ENTRIES)))
            return notice.Render().toJSON()

        # /source/content/\\d+
        num = re.findall(rf'^{URL_CONTENT}/([\d]+)$', CTK.request.url)[0]

        tipe = CTK.cfg.get_val(f'source!{num}!type')
        nick = CTK.cfg.get_val(f'source!{num}!nick')

        cont = CTK.Container()
        cont += CTK.RawHTML (f"<h2>{_('Source')}: {CTK.escape_html(nick)}</h2>")

        workarea = CTK.Box ({'id': 'source-workarea'})

        table = CTK.PropsTable()
        table.Add (_('Type'),       CTK.ComboCfg (f'source!{num}!type', trans_options(SOURCE_TYPES)), _(NOTE_TYPE))
        table.Add (_('Nick'),       CTK.TextCfg (f'source!{num}!nick', False), _(NOTE_NICK))
        table.Add (_('Connection'), CTK.TextCfg (f'source!{num}!host', False), _(NOTE_HOST))

        if tipe == 'interpreter':
            NOTE_INTERPRETER = _(NOTE_INTERPRETER1) + '<br/>' + f'<b>{_(NOTE_INTERPRETER2)}</b>'

            table.Add (_('Interpreter'),         CTK.TextAreaCfg (f'source!{num}!interpreter', False), NOTE_INTERPRETER)
            table.Add (_('Spawning timeout'),    CTK.TextCfg (f'source!{num}!timeout',       True),  _(NOTE_TIMEOUT))
            table.Add (_('Execute as User'),     CTK.TextCfg (f'source!{num}!user',          True),  _(NOTE_USER))
            table.Add (_('Execute as Group'),    CTK.TextCfg (f'source!{num}!group',         True),  _(NOTE_GROUP))
            table.Add (_('Chroot Directory'),    CTK.TextCfg (f'source!{num}!chroot',        True),  _(NOTE_CHROOT))
            table.Add (_('Inherit Environment'), CTK.CheckCfgText (f'source!{num}!env_inherited', True, _('Enabled')), _(NOTE_ENV_INHERIT))

        submit = CTK.Submitter (URL_APPLY)
        submit += table
        workarea += submit

        if CTK.cfg.get_val (f'source!{num}!type') == 'interpreter' and \
           CTK.cfg.get_val (f'source!{num}!env_inherited') == '0':
            workarea += EnvironmentWidget (num)

        sources = _all_sources_per_rule ()
        rules   = [list(dic.keys())[0] for dic in sources if str(num) in list(dic.values())[0]]

        if rules:
            workarea += self.Source_Usage (rules)

        cont += workarea

        render = cont.Render()
        return render.toJSON()


class CloneSource (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)
        self += CTK.RawHTML (_(NOTE_CLONE_DIALOG))


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
                 'cookie_name': COOKIE}

        page += CTK.RawHTML (js=JS_PARTICULAR %(props))
        return page.Render()

class Render:
    class PanelList (CTK.Container):
        def __init__ (self, refresh, right_box):
            CTK.Container.__init__ (self)

            # Helper
            entry = lambda klass, key: CTK.Box ({'class': klass}, CTK.RawHTML (CTK.escape_html (CTK.cfg.get_val(key, ''))))

            # Build the panel list
            panel = SelectionPanel.SelectionPanel (None, right_box.id, "/source", f'{URL_CONTENT}/empty', draggable=False, container='source_panel', cookie_name=COOKIE)
            self += panel

            sources = CTK.cfg.keys('source')
            sources.sort (key=int)

            self.protected_sources = _protected_sources ()

            for k in sources:
                tipe = CTK.cfg.get_val(f'source!{k}!type')
                dialog = self._get_dialog (k, refresh)
                self += dialog

                remove = CTK.ImageStock('del')
                remove.bind ('click', dialog.JS_to_show() + "return false;")

                group = CTK.Box ({'class': 'sel-actions'}, [remove])

                if tipe == 'host':
                    panel.Add (k, f'{URL_CONTENT}/{k}', [group,
                                                             entry('nick',  f'source!{k}!nick'),
                                                             entry('type',  f'source!{k}!type'),
                                                             entry('host',  f'source!{k}!host')])
                elif tipe == 'interpreter':
                    panel.Add (k, f'{URL_CONTENT}/{k}', [group,
                                                             entry('nick',  f'source!{k}!nick'),
                                                             entry('type',  f'source!{k}!type'),
                                                             entry('host',  f'source!{k}!host'),
                                                             entry('inter', f'source!{k}!interpreter')])

        def _get_dialog (self, k, refresh):
            if k in self.protected_sources:
                rules = _rules_per_source (k)

                links = []
                for rule in rules:
                    rule_pre = rule.split('!handler')[0]
                    r = Rule.Rule(f'{rule_pre}!match')
                    rule_name = r.GetName()
                    rule_link = rule_pre.replace('!','/')
                    links.append (LINK_HREF %(rule_link, rule_name))

                dialog  = CTK.Dialog ({'title': _('Deletion is forbidden'), 'width': 480})
                dialog += CTK.RawHTML (_(f"<h2>{_('Configuration consistency')}</h2>"))
                dialog += CTK.RawHTML (_(NOTE_FORBID_1))
                dialog += CTK.RawHTML (f"<p>{_(NOTE_FORBID_2)}: {', '.join(links)}</p>")
                dialog.AddButton (_('Close'), "close")

            else:
                actions = {f'source!{k}':''}
                rule_entries_to_delete = _rules_per_source(k)
                for r in rule_entries_to_delete:
                    actions[r] = ''

                dialog = CTK.Dialog ({'title': _('Do you really want to remove it?'), 'width': 480})
                dialog.AddButton (_('Cancel'), "close")
                dialog.AddButton (_('Remove'), CTK.JS.Ajax (URL_APPLY, async_=False,
                                                            data    = actions,
                                                            success = dialog.JS_to_close() + \
                                                                      refresh.JS_to_refresh()))
                dialog += CTK.RawHTML (_(NOTE_DELETE_DIALOG))

            return dialog



    class PanelButtons (CTK.Box):
        def __init__ (self):
            CTK.Box.__init__ (self, {'class': 'panel-buttons'})

            # Add New
            dialog = CTK.Dialog ({'title': _('Add New Information Source'), 'width': 530})
            dialog.AddButton (_('Cancel'), "close")
            dialog.AddButton (_('Add'), dialog.JS_to_trigger('submit'))
            dialog += AddSource()

            button = CTK.Button('<img src="/static/images/panel-new.png" />', {'id': 'source-new-button', 'class': 'panel-button', 'title': _('Add New Information Source')})
            button.bind ('click', dialog.JS_to_show())
            dialog.bind ('submit_success', dialog.JS_to_close())
            dialog.bind ('submit_success', self.JS_to_trigger('submit_success'));

            self += button
            self += dialog

            # Clone
            dialog = CTK.Dialog ({'title': _('Clone Information Source'), 'width': 480})
            dialog.AddButton (_('Cancel'), "close")
            dialog.AddButton (_('Clone'), JS_CLONE + dialog.JS_to_close())
            dialog += CloneSource()

            button = CTK.Button('<img src="/static/images/panel-clone.png" />', {'id': 'source-clone-button', 'class': 'panel-button', 'title': _('Clone Selected Information Source')})
            button.bind ('click', dialog.JS_to_show())

            self += dialog
            self += button

    def __call__ (self):
        # Content
        left  = CTK.Box({'class': 'panel'})
        left += CTK.RawHTML(f"<h2>{_('Information Sources')}</h2>")

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

CTK.publish (f'^{URL_BASE}$', Render)
CTK.publish (f'^{URL_BASE}/\\d+$', Render_Particular)
CTK.publish (f'^{URL_BASE}/content/[\\d]+$', Render_Source)
CTK.publish (f'^{URL_BASE}/content/empty$', Render_Source)
CTK.publish (f'^{URL_BASE}/content/[\\d]+/clone$', commit_clone)
CTK.publish (f'^{URL_APPLY}', commit, method="POST")
