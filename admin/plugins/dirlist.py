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

import os
import CTK
import Handler
import validations
from configured import *

URL_APPLY     = '/plugin/dirlist/apply'
DEFAULT_THEME = "default"

NOTE_THEME        = N_("Choose the listing theme.")
NOTE_ICON_DIR     = N_("Web directory where the icon files are located. Default: <i>/cherokee_icons</i>.")
NOTE_NOTICE_FILES = N_("List of notice files to be inserted.")
NOTE_HIDDEN_FILES = N_("List of files that should not be listed.")

HELPS = [('modules_handlers_dirlist', N_("Only listing"))]


class Plugin_dirlist (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        symlinks = kwargs.pop('symlinks', True)

        # Listing
        table = CTK.PropsTable()
        table.Add (_('Show Size'),               CTK.CheckCfgText("%s!size"%(self.key),           True,  _('Show')),    '')
        table.Add (_('Show Date'),               CTK.CheckCfgText("%s!date"%(self.key),           True,  _('Show')),    '')
        table.Add (_('Show User'),               CTK.CheckCfgText("%s!user"%(self.key),           False, _('Show')),    '')
        table.Add (_('Show Group'),              CTK.CheckCfgText("%s!group"%(self.key),          False, _('Show')),    '')
        table.Add (_('Show Backup files'),       CTK.CheckCfgText("%s!backup"%(self.key),         False, _('Show')),    '')
        table.Add (_('Show Hidden files'),       CTK.CheckCfgText("%s!hidden"%(self.key),         False, _('Show')),    '')
        if symlinks:
            table.Add (_('Allow symbolic links'),    CTK.CheckCfgText("%s!symlinks"%(self.key),       True,  _('Allow')),   '')
        table.Add (_('Redirect symbolic links'), CTK.CheckCfgText("%s!redir_symlinks"%(self.key), False, _('Enabled')), '')

        submit = CTK.Submitter (URL_APPLY)
        submit += table
        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Listing')))
        self += CTK.Indenter (submit)

        # Theming
        table = CTK.PropsTable()
        table.Add (_('Theme'),        CTK.ComboCfg("%s!theme"%(key), self._get_theme_list()), _(NOTE_THEME))
        table.Add (_('Icons dir'),    CTK.TextCfg("%s!icon_dir"%(key), True), _(NOTE_ICON_DIR))
        table.Add (_('Notice files'), CTK.TextCfg("%s!notice_files"%(key), True), _(NOTE_NOTICE_FILES))
        table.Add (_('Hidden files'), CTK.TextCfg("%s!hidden_files"%(key), True), _(NOTE_HIDDEN_FILES))

        submit = CTK.Submitter (URL_APPLY)
        submit += table
        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Theming')))
        self += CTK.Indenter (submit)

        # Publish
        VALS = [("%s!icon_dir"%(key),     validations.is_path),
                ("%s!notice_files"%(key), validations.is_path_list),
                ("%s!hidden_files"%(key), validations.is_list)]

        CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, validation=VALS, method="POST")

    def _get_theme_list (self):
        themes = []
        for f in os.listdir(CHEROKEE_THEMEDIR):
            full = os.path.join(CHEROKEE_THEMEDIR, f)
            if os.path.isdir (full):
                themes.append((f,f))

        if not themes:
            themes = [(DEFAULT_THEME, DEFAULT_THEME)]
        return themes
