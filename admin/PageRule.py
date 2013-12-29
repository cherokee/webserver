# -*- coding: utf-8 -*-
#
# Cherokee-admin
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
import Page
import Cherokee
import SelectionPanel
import validations
import Wizard

from Rule          import Rule
from CTK.Tab       import HEADER as Tab_HEADER
from CTK.Submitter import HEADER as Submit_HEADER
from CTK.TextField import HEADER as TextField_HEADER
from CTK.SortableList import HEADER as SortableList_HEADER

from util import *
from consts import *
from CTK.util import *
from CTK.consts import *
from configured import *

URL_BASE         = '/vserver/%s/rule'
URL_APPLY        = '/vserver/%s/rule/apply'
URL_NEW_MANUAL   = '/vserver/%s/rule/new/manual'
URL_NEW_MANUAL_R = r'/vserver/(\d+)/rule/new/manual'
URL_BASE_R       = r'^/vserver/(\d+)/rule$'
URL_APPLY_R      = r'^/vserver/(\d+)/rule/apply'
URL_PARTICULAR_R = r'^/vserver/(\d+)/rule/\d+$'

NOTE_DELETE_DIALOG = N_('<p>You are about to delete the <b>%s</b> behavior rule.</p><p>Are you sure you want to proceed?</p>')
NOTE_CLONE_DIALOG  = N_('You are about to clone a Behavior Rule. Would you like to proceed?')

HELPS = []
VALIDATIONS = []

JS_ACTIVATE_LAST = """
$('.selection-panel:first').data('selectionpanel').select_last();
"""

JS_CLONE = """
  var panel = $('.selection-panel:first').data('selectionpanel').get_selected();
  var url   = panel.find('.row_content').attr('url');

  $.ajax ({type: 'GET', async: false, url: url+'/clone', success: function(data) {
      $('.panel-buttons').trigger ('submit_success');
  }});
"""

JS_PARTICULAR = """
  var vserver = window.location.pathname.match (/^\/vserver\/(\d+)/)[1];
  var rule    = window.location.pathname.match (/^\/vserver\/\d+\/rule\/(\d+)/)[1];

  $.cookie ('%(cookie_name)s', rule+'_'+vserver, { path: '/vserver/'+ vserver + '/rule'});
  window.location.replace ('/vserver/'+ vserver + '/rule');
"""


def Commit():
    # Modifications
    return CTK.cfg_apply_post()


def reorder (arg):
    # Process new list
    order = CTK.post.pop(arg)

    tmp = order.split(',')
    vsrv = tmp[0].split('_')[1]

    tmp = [x.split('_')[0] for x in tmp]
    tmp.reverse()

    # Build and alternative tree
    num = 100
    for r in tmp:
        CTK.cfg.clone ('vserver!%s!rule!%s'%(vsrv, r), 'tmp!vserver!%s!rule!%d'%(vsrv, num))
        num += 100

    # Set the new list in place
    del (CTK.cfg['vserver!%s!rule'%(vsrv)])
    CTK.cfg.rename ('tmp!vserver!%s!rule'%(vsrv), 'vserver!%s!rule'%(vsrv))
    return CTK.cfg_reply_ajax_ok()


def NewManual():
    # Figure Virtual Server number
    vsrv_num = re.findall (URL_NEW_MANUAL_R, CTK.request.url)[0]

    # Add New Rule: Content
    rules = [('',_('Choose'))] + trans_options(RULES)

    table = CTK.PropsTable()
    modul = CTK.PluginSelector ('tmp', rules, vsrv_num=vsrv_num)
    table.Add (_('Rule Type'), modul.selector_widget, '')

    box = CTK.Box()
    box += table
    box += modul
    return box.Render().toJSON()


class RuleNew (CTK.Container):
    def __init__ (self, vsrv_num):
        CTK.Container.__init__ (self)

        # Build the panel list
        right_box = CTK.Box({'class': 'rule_new_content'})
        panel = SelectionPanel.SelectionPanel (None, right_box.id, URL_BASE%(vsrv_num), '', cookie_name='new_rule_selected')

        self += panel
        self += right_box

        # Special 1st: Manual
        content = [CTK.Box({'class': 'title'},       CTK.RawHTML(_('Manual'))),
                   CTK.Box({'class': 'description'}, CTK.RawHTML(_('Manual configuration')))]
        panel.Add ('manual', URL_NEW_MANUAL%(vsrv_num), content, draggable=False)


        # Wizard Categories
        wizards2_path = os.path.realpath (__file__ + "/../wizards2")
        if os.path.exists (wizards2_path):
            # Wizards 2.0
            import wizards2

            categories = wizards2.Categories.get()
            for cat in categories:
                url_pre = '%s/%s' %(wizards2.Categories.URL_CAT_LIST_RULE, categories.index(cat))
                content = [CTK.Box({'class': 'title'}, CTK.RawHTML(_(cat)))]
                panel.Add (cat.replace(' ','_'), url_pre, content, draggable=False)
        else:
            # Wizards 1.0
            for cat in Wizard.Categories (Wizard.TYPE_RULE):
                url_pre = '%s/%s' %(Wizard.URL_CAT_LIST_RULE, cat['name'])
                title, descr = cat['title'], cat['descr']
                content = [CTK.Box({'class': 'title'},       CTK.RawHTML(_(title))),
                           CTK.Box({'class': 'description'}, CTK.RawHTML(_(descr)))]
                panel.Add (cat['name'], url_pre, content, draggable=False)


class Render:
    class PanelList (CTK.Container):
        def __init__ (self, refresh, right_box, vsrv_num):
            CTK.Container.__init__ (self)
            url_base  = '/vserver/%s/rule' %(vsrv_num)
            url_apply = URL_APPLY %(vsrv_num)

            # Build the panel list
            panel = SelectionPanel.SelectionPanel (reorder, right_box.id, url_base, '', container='rules_panel')
            self += panel

            # Build the Rule list
            rules = CTK.cfg.keys('vserver!%s!rule'%(vsrv_num))
            rules.sort (lambda x,y: cmp(int(x), int(y)))
            rules.reverse()

            for r in rules:
                rule = Rule ('vserver!%s!rule!%s!match'%(vsrv_num, r))
                rule_name = rule.GetName()
                rule_name_esc = CTK.escape_html (rule_name)

                # Comment
                comment = []

                handler = CTK.cfg.get_val ('vserver!%s!rule!%s!handler' %(vsrv_num, r))
                if handler:
                    desc = filter (lambda x: x[0] == handler, HANDLERS)[0][1]
                    comment.append (_(desc))

                auth = CTK.cfg.get_val ('vserver!%s!rule!%s!auth' %(vsrv_num, r))
                if auth:
                    desc = filter (lambda x: x[0] == auth, VALIDATORS)[0][1]
                    comment.append (_(desc))

                for e in CTK.cfg.keys ('vserver!%s!rule!%s!encoder'%(vsrv_num, r)):
                    val = CTK.cfg.get_val ('vserver!%s!rule!%s!encoder!%s'%(vsrv_num, r, e))
                    if val == 'allow':
                        comment.append (e)
                    elif val == 'forbid':
                        comment.append ("no %s"%(e))

                if CTK.cfg.get_val ('vserver!%s!rule!%s!flcache'%(vsrv_num, r)) == "allow":
                    comment.append ('Cache')

                if CTK.cfg.get_val ('vserver!%s!rule!%s!timeout' %(vsrv_num, r)):
                    comment.append ('Timeout')

                if CTK.cfg.get_val ('vserver!%s!rule!%s!rate' %(vsrv_num, r)):
                    comment.append ('Traffic')

                if int (CTK.cfg.get_val ('vserver!%s!rule!%s!no_log'%(vsrv_num, r), "0")) > 0:
                    comment.append ('No log')

                # List entry
                row_id = '%s_%s' %(r, vsrv_num)

                if r == rules[-1]:
                    content = [CTK.Box ({'class': 'name'}, CTK.RawHTML (rule_name_esc)),
                               CTK.Box ({'class': 'comment'}, CTK.RawHTML (', '.join(comment)))]
                    panel.Add (row_id, '/vserver/%s/rule/content/%s'%(vsrv_num, r), content, draggable=False)
                else:
                    # Remove
                    dialog = CTK.Dialog ({'title': _('Do you really want to remove it?'), 'width': 480})
                    dialog.AddButton (_('Cancel'), "close")
                    dialog.AddButton (_('Remove'), CTK.JS.Ajax (url_apply, async=False,
                                                                data    = {'vserver!%s!rule!%s'%(vsrv_num, r):''},
                                                                success = dialog.JS_to_close() + \
                                                                          refresh.JS_to_refresh()))
                    dialog += CTK.RawHTML (_(NOTE_DELETE_DIALOG) %(rule_name_esc))
                    self += dialog
                    remove = CTK.ImageStock('del')
                    remove.bind ('click', dialog.JS_to_show() + "return false;")

                    # Disable
                    is_disabled = bool (int (CTK.cfg.get_val('vserver!%s!rule!%s!disabled'%(vsrv_num,r), "0")))
                    disclass = ('','rule-inactive')[is_disabled][:]

                    disabled = CTK.ToggleButtonOnOff (not is_disabled)
                    disabled.bind ('changed',
                                   CTK.JS.Ajax (url_apply, async=True,
                                                data = '{"vserver!%s!rule!%s!disabled": event.value}'%(vsrv_num,r)))
                    disabled.bind ('changed',
                                   "$(this).parents('.row_content').toggleClass('rule-inactive');")

                    # Final
                    is_final = bool (int (CTK.cfg.get_val('vserver!%s!rule!%s!match!final'%(vsrv_num,r), "1")))

                    final = CTK.ToggleButton (CTK.Box({'class': 'final-on'},  CTK.RawHTML(_('Final'))),
                                              CTK.Box({'class': 'final-off'}, CTK.RawHTML(_('Non Final'))),
                                              is_final)
                    final.bind ('changed',
                                CTK.JS.Ajax (url_apply, async=True,
                                             data = '{"vserver!%s!rule!%s!match!final": parseInt(event.value)?"0":"1"}'%(vsrv_num,r)))

                    # Actions
                    group = CTK.Box ({'class': 'sel-actions'}, [disabled, remove])

                    content = [group]
                    content += [CTK.Box ({'class': 'name'},    CTK.RawHTML (rule_name_esc)),
                                CTK.Box ({'class': 'final'},   final),
                                CTK.Box ({'class': 'comment'}, CTK.RawHTML (', '.join(comment)))]


                    # Add the list entry
                    panel.Add (row_id, '/vserver/%s/rule/content/%s'%(vsrv_num, r), content, True, disclass)


    class PanelButtons (CTK.Box):
        def __init__ (self, vsrv_num):
            CTK.Box.__init__ (self, {'class': 'panel-buttons'})

            # Add New
            dialog = CTK.Dialog ({'title': _('Add Behavior Rule'), 'width': 720})
            dialog.id = 'dialog-new-rule'
            dialog.AddButton (_('Cancel'), "close")
            dialog.AddButton (_('Add'), dialog.JS_to_trigger('submit'))
            dialog += RuleNew (vsrv_num)

            druid  = CTK.Druid (CTK.RefreshableURL())
            wizard = CTK.Dialog ({'title': _('Configuration Assistant'), 'width': 550})
            wizard += druid
            druid.bind ('druid_exiting',
                        wizard.JS_to_close() +
                        self.JS_to_trigger('submit_success'))

            button = CTK.Button('<img src="/static/images/panel-new.png" />', {'id': 'rule-new-button', 'class': 'panel-button', 'title': _('Add Behavior Rule')})
            button.bind ('click', dialog.JS_to_show())
            dialog.bind ('submit_success', dialog.JS_to_close())
            dialog.bind ('submit_success', self.JS_to_trigger('submit_success'));
            dialog.bind ('open_wizard',
                         dialog.JS_to_close() +
                         druid.JS_to_goto("'/wizard/vserver/%s/' + event.wizard" %(vsrv_num)) +
                         wizard.JS_to_show())

            self += button
            self += dialog
            self += wizard

            # Clone
            dialog = CTK.Dialog ({'title': _('Clone Behavior Rule'), 'width': 480})
            dialog.AddButton (_('Cancel'), "close")
            dialog.AddButton (_('Clone'), JS_CLONE + dialog.JS_to_close())
            dialog += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_CLONE_DIALOG)))

            button = CTK.Button('<img src="/static/images/panel-clone.png" />', {'id': 'rule-clone-button', 'class': 'panel-button', 'title': _('Clone Selected Behavior Rule')})
            button.bind ('click', dialog.JS_to_show())

            self += dialog
            self += button


    def __call__ (self):
        title = _('Behavior')
        vsrv_num = re.findall (URL_BASE_R, CTK.request.url)[0]

        # Ensure the VServer exists
        if not CTK.cfg.keys('vserver!%s'%(vsrv_num)):
            return CTK.HTTP_Redir ('/vserver')

        # Content
        left  = CTK.Box({'class': 'panel'})
        left += CTK.RawHTML('<h2>%s</h2>'%(title))

        # Virtual Server List
        refresh = CTK.Refreshable ({'id': 'rules_panel'})
        refresh.register (lambda: self.PanelList(refresh, right, vsrv_num).Render())

        # Refresh on 'New' or 'Clone'
        buttons = self.PanelButtons (vsrv_num)
        buttons.bind ('submit_success', refresh.JS_to_refresh (on_success=JS_ACTIVATE_LAST))
        left += buttons

        left += CTK.Box({'class': 'filterbox'}, CTK.TextField({'class':'filter', 'optional_string': _('Rule Filtering'), 'optional': True}))
        right = CTK.Box({'class': 'rules_content'})
        left += refresh

        # Refresh the list whenever the content change
        right.bind ('changed',          refresh.JS_to_refresh());
        right.bind ('update_rule_list', refresh.JS_to_refresh());

        # Refresh the list when it's been reordered
        left.bind ('reordered', refresh.JS_to_refresh())

        # Build the page
        headers = Tab_HEADER + Submit_HEADER + TextField_HEADER + SortableList_HEADER
        page = Page.Base(title, body_id='rules', helps=HELPS, headers=headers)
        page += left
        page += right

        return page.Render()


class RenderParticular:
    def __call__ (self):
        headers = ['<script type="text/javascript" src="/CTK/js/jquery-1.3.2.min.js"></script>',
                   '<script type="text/javascript" src="/CTK/js/jquery.cookie.js"></script>']

        page  = CTK.PageEmpty (headers=headers)
        props = {'cookie_name': SelectionPanel.COOKIE_NAME_DEFAULT}
        page += CTK.RawHTML (HTML_JS_BLOCK %(JS_PARTICULAR %(props)))

        return page.Render()


CTK.publish (URL_BASE_R,       Render)
CTK.publish (URL_PARTICULAR_R, RenderParticular)
CTK.publish (URL_APPLY_R,      Commit, method="POST", validation=VALIDATIONS)
CTK.publish (URL_NEW_MANUAL_R, NewManual)
