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

from CTK.Tab       import HEADER as Tab_HEADER
from CTK.Submitter import HEADER as Submit_HEADER
from CTK.TextField import HEADER as TextField_HEADER
from CTK.SortableList import HEADER as SortableList_HEADER

from util import *
from consts import *
from CTK.util import *
from CTK.consts import *
from configured import *


URL_BASE       = r'/vserver'
URL_APPLY      = r'/vserver/apply'
URL_NEW_MANUAL = r'/vserver/new/manual'

HELPS = [('config_virtual_servers', N_("Virtual Servers"))]

NOTE_DELETE_DIALOG = N_('<p>You are about to delete the <b>%(nick_esc)s</b> Virtual Server.</p><p>Are you sure you want to proceed?</p>')
NOTE_CLONE_DIALOG  = N_('You are about to clone a Virtual Server. Would you like to proceed?')
NOTE_NEW_NICK      = N_('Name of the Virtual Server you are about to create. A domain name is alright.')
NOTE_NEW_DROOT     = N_('Document Root directory of the new Virtual Server.')

VALIDATIONS = [
    ('tmp!new_droot', validations.is_dev_null_or_local_dir_exists),
    ('tmp!new_nick',  validations.is_new_vserver_nick)
]

JS_ACTIVATE_FIRST = """
$('#%s').find('.selection-panel:first').data('selectionpanel').select_first();
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
  var vserver = window.location.pathname.match (/^\/vserver\/(\d+)\/?$/)[1];
  $.cookie ('%(cookie_name)s', vserver, { path: '/vserver' });
  window.location.replace ('%(url_base)s'+window.location.hash);
"""

def Commit():
    # New Virtual Server
    new_nick  = CTK.post.pop('tmp!new_nick')
    new_droot = CTK.post.pop('tmp!new_droot')

    if new_nick and new_droot:
        next = CTK.cfg.get_next_entry_prefix ('vserver')
        CTK.cfg['%s!nick'                   %(next)] = new_nick
        CTK.cfg['%s!document_root'          %(next)] = new_droot
        CTK.cfg['%s!rule!1!match'           %(next)] = 'default'
        CTK.cfg['%s!rule!1!handler'         %(next)] = 'common'

        CTK.cfg['%s!rule!2!match'           %(next)] = 'directory'
        CTK.cfg['%s!rule!2!match!directory' %(next)] = '/cherokee_icons'
        CTK.cfg['%s!rule!2!handler'         %(next)] = 'file'
        CTK.cfg['%s!rule!2!document_root'   %(next)] = CHEROKEE_ICONSDIR

        CTK.cfg['%s!rule!3!match'           %(next)] = 'directory'
        CTK.cfg['%s!rule!3!match!directory' %(next)] = '/cherokee_themes'
        CTK.cfg['%s!rule!3!handler'         %(next)] = 'file'
        CTK.cfg['%s!rule!3!document_root'   %(next)] = CHEROKEE_THEMEDIR

        return CTK.cfg_reply_ajax_ok()

    # Modifications
    return CTK.cfg_apply_post()


def reorder (arg):
    # Process new list
    order = CTK.post.pop(arg)
    tmp = order.split(',')
    tmp.reverse()

    # Build and alternative tree
    num = 10
    for v in tmp:
        CTK.cfg.clone ('vserver!%s'%(v), 'tmp!vserver!%d'%(num))
        num += 10

    # Set the new list in place
    del (CTK.cfg['vserver'])
    CTK.cfg.rename ('tmp!vserver', 'vserver')
    return CTK.cfg_reply_ajax_ok()


def NewManual():
    table = CTK.PropsTable()
    table.Add (_('Nick'),          CTK.TextCfg ('tmp!new_nick',  False, {'class': 'noauto'}), _(NOTE_NEW_NICK))
    table.Add (_('Document Root'), CTK.TextCfg ('tmp!new_droot', False, {'class': 'noauto'}), _(NOTE_NEW_DROOT))

    submit = CTK.Submitter (URL_APPLY)
    submit += table

    box = CTK.Box({'class': 'vserver-new-box'})
    box += CTK.RawHTML ('<h2>%s</h2>' %(_('Manual')))
    box += submit

    return box.Render().toJSON()


class VirtualServerNew (CTK.Container):
    def __init__ (self):
        CTK.Container.__init__ (self)

        # Build the panel list
        right_box = CTK.Box({'class': 'vserver_new_content'})
        panel = SelectionPanel.SelectionPanel (None, right_box.id, URL_BASE, '', draggable=False, cookie_name='new_vsrv_selected')

        self += panel
        self += right_box

        # Special 1st: Manual
        content = [CTK.Box({'class': 'title'},       CTK.RawHTML(_('Manual'))),
                   CTK.Box({'class': 'description'}, CTK.RawHTML(_('Manual configuration')))]
        panel.Add ('manual', URL_NEW_MANUAL, content, draggable=False)

        # Wizard Categories
        wizards2_path = os.path.realpath (__file__ + "/../wizards2")
        if os.path.exists (wizards2_path):
            # Wizards 2.0
            import wizards2

            categories = wizards2.Categories.get()
            for cat in categories:
                url_pre = '%s/%s' %(wizards2.Categories.URL_CAT_LIST_VSRV, categories.index(cat))
                content = [CTK.Box({'class': 'title'}, CTK.RawHTML(_(cat)))]
                panel.Add (cat.replace(' ','_'), url_pre, content, draggable=False)
        else:
            # Wizards 1.0
            for cat in Wizard.Categories (Wizard.TYPE_VSERVER):
                url_pre = '%s/%s' %(Wizard.URL_CAT_LIST_VSRV, cat['name'])
                title, descr = cat['title'], cat['descr']
                content = [CTK.Box({'class': 'title'},       CTK.RawHTML(_(title))),
                           CTK.Box({'class': 'description'}, CTK.RawHTML(_(descr)))]
                panel.Add (cat['name'], url_pre, content, draggable=False)


class Render:
    class PanelList (CTK.Container):
        def __init__ (self, refresh, right_box):
            CTK.Container.__init__ (self)

            # Sanity check
            if not CTK.cfg.keys('vserver'):
                CTK.cfg['vserver!1!nick']           = 'default'
                CTK.cfg['vserver!1!document_root']  = '/tmp'
                CTK.cfg['vserver!1!rule!1!match']   = 'default'
                CTK.cfg['vserver!1!rule!1!handler'] = 'common'

            # Helper
            entry = lambda klass, key: CTK.Box ({'class': klass}, CTK.RawHTML (CTK.escape_html (CTK.cfg.get_val(key, ''))))

            # Build the panel list
            panel = SelectionPanel.SelectionPanel (reorder, right_box.id, URL_BASE, '', container='vservers_panel')
            self += panel

            # Build the Virtual Server list
            vservers = CTK.cfg.keys('vserver')
            vservers.sort (lambda x,y: cmp(int(x), int(y)))
            vservers.reverse()

            for k in vservers:
                # Document root widget
                droot_str = CTK.cfg.get_val ('vserver!%s!document_root'%(k), '')
                droot_widget = CTK.Box ({'class': 'droot'}, CTK.RawHTML(CTK.escape_html (droot_str)))

                if k == vservers[-1]:
                    content = [entry('nick', 'vserver!%s!nick'%(k)), droot_widget]
                    panel.Add (k, '/vserver/content/%s'%(k), content, draggable=False)
                else:
                    nick     = CTK.cfg.get_val ('vserver!%s!nick'%(k), _('Unknown'))
                    nick_esc = CTK.escape_html (nick)

                    # Remove
                    dialog = CTK.Dialog ({'title': _('Do you really want to remove it?'), 'width': 480})
                    dialog.AddButton (_('Cancel'), "close")
                    dialog.AddButton (_('Remove'), CTK.JS.Ajax (URL_APPLY, async=True,
                                                                data    = {'vserver!%s'%(k):''},
                                                                success = dialog.JS_to_close() + \
                                                                    refresh.JS_to_refresh()))
                    dialog += CTK.RawHTML (_(NOTE_DELETE_DIALOG) %(locals()))
                    self += dialog
                    remove = CTK.ImageStock('del')
                    remove.bind ('click', dialog.JS_to_show() + "return false;")

                    # Disable
                    is_disabled = bool (int (CTK.cfg.get_val('vserver!%s!disabled'%(k), "0")))
                    disclass = ('','vserver-inactive')[is_disabled][:]

                    disabled = CTK.ToggleButtonOnOff (not is_disabled)
                    disabled.bind ('changed',
                                   CTK.JS.Ajax (URL_APPLY, async=True,
                                                data = '{"vserver!%s!disabled": event.value}'%(k)))
                    disabled.bind ('changed',
                                   "$(this).parents('.row_content').toggleClass('vserver-inactive');")

                    # Actions
                    group = CTK.Box ({'class': 'sel-actions'}, [disabled, remove])

                    content = [group]
                    content += [entry('nick', 'vserver!%s!nick'%(k)), droot_widget]

                    # List entry
                    panel.Add (k, '/vserver/content/%s'%(k), content, True, disclass)

    class PanelButtons (CTK.Box):
        def __init__ (self):
            CTK.Box.__init__ (self, {'class': 'panel-buttons'})

            # Add New
            dialog = CTK.Dialog ({'title': _('Add New Virtual Server'), 'width': 720})
            dialog.id = 'dialog-new-vserver'
            dialog.AddButton (_('Cancel'), "close")
            dialog.AddButton (_('Add'), dialog.JS_to_trigger('submit'))
            dialog += VirtualServerNew()

            druid  = CTK.Druid (CTK.RefreshableURL())
            wizard = CTK.Dialog ({'title': _('Virtual Server Configuration Assistant'), 'width': 550})
            wizard += druid
            druid.bind ('druid_exiting',
                        wizard.JS_to_close() +
                        self.JS_to_trigger('submit_success'))

            button = CTK.Button('<img src="/static/images/panel-new.png" />', {'id': 'vserver-new-button', 'class': 'panel-button', 'title': _('Add New Virtual Server')})
	    # JS_ACTIVATE_FIRST will move the left selection pane inside the dialog up.
            button.bind ('click',
                         JS_ACTIVATE_FIRST %(dialog.id) +
                         dialog.JS_to_show())
            dialog.bind ('submit_success', dialog.JS_to_close())
            dialog.bind ('submit_success', self.JS_to_trigger('submit_success'))
            dialog.bind ('open_wizard',
                         dialog.JS_to_close() +
                         druid.JS_to_goto("'/wizard/vserver/' + event.wizard") +
                         wizard.JS_to_show())

            self += button
            self += dialog
            self += wizard

            # Clone
            dialog = CTK.Dialog ({'title': _('Clone Virtual Server'), 'width': 480})
            dialog.AddButton (_('Cancel'), "close")
            dialog.AddButton (_('Clone'), JS_CLONE + dialog.JS_to_close())
            dialog += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_CLONE_DIALOG)))

            button = CTK.Button('<img src="/static/images/panel-clone.png" />', {'id': 'vserver-clone-button', 'class': 'panel-button', 'title': _('Clone Selected Virtual Server')})
            button.bind ('click', dialog.JS_to_show())

            self += dialog
            self += button

    CHILD_HEADERS = None

    def __call__ (self):
        title = _('Virtual Servers')

        # Virtual Server List
        left  = CTK.Box({'class': 'panel'})
        left += CTK.RawHTML('<h2>%s</h2>'%(title))

        # Content
        refresh_r = CTK.Refreshable ({'id': 'vservers_panel'})
        refresh_r.register (lambda: self.PanelList(refresh_r, right).Render())

        # Refresh on 'New' or 'Clone'
        buttons = self.PanelButtons()
        buttons.bind ('submit_success',
                      refresh_r.JS_to_refresh (on_success=JS_ACTIVATE_FIRST %(refresh_r.id)))
        left += buttons

        left += CTK.Box({'class': 'filterbox'}, CTK.TextField({'class':'filter', 'optional_string': _('Virtual Server Filtering'), 'optional': True}))
        right = CTK.Box({'class': 'vserver_content'})
        left += refresh_r

        # Refresh the list whenever the content change
        right.bind ('submit_success', refresh_r.JS_to_refresh());

        # Refresh the list when it's been reordered
        left.bind ('reordered', refresh_r.JS_to_refresh())

        # Figure out content panel headers. This step is very tricky.
        # We have no idea what HTML headers the content HTML will
        # have. In fact, the panel content is receiving only the HTML
        # content only (body).
        #
        # The static CHILD_HEADERS class property will hold a copy of
        # the headers generated by the direct render of the PageVServer
        # class. It should cover most of the cases, unless a dynamically
        # loaded module requires an additional header entry.
        #
        if not self.CHILD_HEADERS:
            import PageVServer
            vsrv_num = CTK.cfg.keys('vserver')[0]
            render   = PageVServer.RenderContent(vsrv_num).Render()
            self.CHILD_HEADERS = render.headers

        # Build the page
        page = Page.Base(title, body_id='vservers', helps=HELPS, headers=self.CHILD_HEADERS)
        page += left
        page += right

        return page.Render()


class RenderParticular:
    def __call__ (self):
        headers = SelectionPanel.HEADER
        page    = CTK.Page(headers=headers)

        props = {'url_base':    URL_BASE,
                 'cookie_name': SelectionPanel.COOKIE_NAME_DEFAULT}
        page += CTK.RawHTML (js=JS_PARTICULAR %(props))

        return page.Render()


CTK.publish (r'^%s$'    %(URL_BASE),       Render)
CTK.publish (r'^%s/\d+$'%(URL_BASE),       RenderParticular)
CTK.publish (r'^%s$'    %(URL_NEW_MANUAL), NewManual, method="POST", validation=VALIDATIONS)
CTK.publish (r'^%s'     %(URL_APPLY),      Commit,    method="POST", validation=VALIDATIONS)

