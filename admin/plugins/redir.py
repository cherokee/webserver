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
import Handler

from util import *
from consts import *

URL_APPLY = '/plugin/redir/apply'

NOTE_SHOW         = N_("Defines whether the redirection will be seen by the client.")
NOTE_REGEX        = N_('Regular expression. Check out the <a target="_blank" href="http://perldoc.perl.org/perlre.html">Reference</a>.')
NOTE_SUBSTITUTION = N_("Target address. It can use Regular Expression substitution sub-strings.")
NOTE_EMPTY        = N_("At least one Regular Expression should be defined.")

HELPS = [('modules_handlers_redir',              N_("Redirections")),
         ('http://perldoc.perl.org/perlre.html', N_("Regular Expressions"))]


def commit():
    new_show  = CTK.post.pop('tmp!new_show')
    new_regex = CTK.post.pop('tmp!new_regex')
    new_subst = CTK.post.pop('tmp!new_subst')
    key       = CTK.post.pop('key')

    # New
    if new_regex or new_subst:
        next = CTK.cfg.get_next_entry_prefix (key)
        CTK.cfg['%s!show'%(next)] = new_show
        if new_regex:
            CTK.cfg['%s!regex'%(next)] = new_regex
        if new_subst:
            CTK.cfg['%s!substring'%(next)] = new_subst
        return CTK.cfg_reply_ajax_ok()

    # Modification
    return CTK.cfg_apply_post()


class Plugin_redir (Handler.PluginHandler):
    class Content (CTK.Box):
        def __init__ (self, refresh, key):
            CTK.Box.__init__ (self)

            CTK.cfg.normalize (key)
            keys = CTK.cfg.keys (key)
            keys.sort(lambda x,y: cmp(int(x), int(y)))

            self += CTK.RawHTML ("<h3>%s</h3>" % (_('Rule list')))
            if not keys:
                self += CTK.Indenter (CTK.Notice ('information', CTK.RawHTML(_(NOTE_EMPTY))))
            else:
                table = CTK.SortableList (lambda arg: CTK.SortableList__reorder_generic (arg, key), self.id)
                table += [CTK.RawHTML(x) for x in ('', _('Type'), _('Regular Expression'), _('Substitution'))]
                table.set_header(1)

                for k in keys:
                    show  = CTK.ComboCfg('%s!%s!show'     %(key, k), trans_options(REDIR_SHOW))
                    regex = CTK.TextCfg ('%s!%s!regex'    %(key, k))
                    subst = CTK.TextCfg ('%s!%s!substring'%(key, k))

                    remove = None
                    if len(keys) >= 2:
                        remove = CTK.ImageStock('del')
                        remove.bind('click', CTK.JS.Ajax (URL_APPLY, data={'%s!%s'%(key, k): ''},
                                                          complete = refresh.JS_to_refresh()))

                    table += [None, show, regex, subst, remove]

                    table[-1].props['id']       = k
                    table[-1][1].props['class'] = 'dragHandle'

                submit = CTK.Submitter (URL_APPLY)
                submit += table
                self += CTK.Indenter (submit)

    class AddNew_Button (CTK.Box):
        class Content (CTK.Container):
            def __init__ (self, key):
                CTK.Container.__init__ (self)

                table = CTK.PropsTable()
                table.Add (_('Show'),               CTK.ComboCfg('tmp!new_show', trans_options(REDIR_SHOW), {'class': 'noauto'}), _(NOTE_SHOW))
                table.Add (_('Regular Expression'), CTK.TextCfg('tmp!new_regex', False, {'class': 'noauto'}), _(NOTE_REGEX))
                table.Add (_('Substitution'),       CTK.TextCfg('tmp!new_subst', False, {'class': 'noauto'}), _(NOTE_SUBSTITUTION))

                submit = CTK.Submitter(URL_APPLY)
                submit += CTK.Hidden ('key', '%s!rewrite'%(key))
                submit += table
                self += submit

        def __init__ (self, key):
            CTK.Box.__init__ (self, {'class': 'mime-button'})

            # Add New
            dialog = CTK.Dialog ({'title': _('Add New Regular Expression'), 'width': 540})
            dialog.AddButton (_('Add'), dialog.JS_to_trigger('submit'))
            dialog.AddButton (_('Cancel'), "close")
            dialog += self.Content (key)

            button = CTK.Button(_('Add New RegEx'))
            button.bind ('click', dialog.JS_to_show())
            dialog.bind ('submit_success', dialog.JS_to_close())
            dialog.bind ('submit_success', self.JS_to_trigger('submit_success'));

            self += button
            self += dialog

    def __init__ (self, key, **kwargs):
        kwargs['show_document_root'] = False
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        # List
        refresh = CTK.Refreshable ({'id': 'plugin_redir'})
        refresh.register (lambda: self.Content(refresh, '%s!rewrite'%(key)).Render())
        self += refresh

        # New
        button = self.AddNew_Button (key)
        button.bind ('submit_success', refresh.JS_to_refresh ())
        self += CTK.Indenter (button)


CTK.publish ('^%s'%(URL_APPLY), commit, method="POST")
