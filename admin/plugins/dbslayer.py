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
import Cherokee
import Balancer

from util import *
from consts import *

URL_APPLY = '/plugin/dbslayer/apply'
HELPS     = [('modules_handlers_dbslayer', N_("MySQL balancing")),
             ('cookbook_dbslayer', N_("DB balancig recipe"))]

NOTE_LANG     = N_("Language from which the information will be consumed.")
NOTE_USER     = N_("User to access the database.")
NOTE_PASSWORD = N_("Password for the user accessing the database.")
NOTE_DB       = N_("Database to connect to.")


class Plugin_dbslayer (Handler.PluginHandler):
    def __init__ (self, key, **kwargs):
        kwargs['show_document_root'] = False
        Handler.PluginHandler.__init__ (self, key, **kwargs)
        Handler.PluginHandler.AddCommon (self)

        # DB-Slayer
        table = CTK.PropsTable()
        table.Add (_('Language'),    CTK.ComboCfg('%s!lang'%(key),    trans_options(DWRITER_LANGS)), _(NOTE_LANG))
        table.Add (_('DB User'),     CTK.TextCfg('%s!user'%(key),     False), _(NOTE_USER))
        table.Add (_('DB Password'), CTK.TextCfg('%s!password'%(key), False), _(NOTE_PASSWORD))
        table.Add (_('Data Base'),   CTK.TextCfg('%s!db'%(key),       True),  _(NOTE_DB))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        self += CTK.RawHTML ("<h2>%s</h2>" %(_('Serialization')))
        self += CTK.Indenter (submit)

        # Load Balancing
        modul = CTK.PluginSelector('%s!balancer'%(key), trans_options(Cherokee.support.filter_available (BALANCERS)))
        table = CTK.PropsTable()
        table.Add (_("Balancer"), modul.selector_widget, _(Balancer.NOTE_BALANCER))

        self += CTK.RawHTML ('<h2>%s</h2>' %(_('Data Base Balancing')))
        self += CTK.Indenter (table)
        self += modul

CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, method="POST")
