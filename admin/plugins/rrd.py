# -*- coding: utf-8 -*-
#
# Cheroke-admin
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
import validations

URL_APPLY = '/plugin/rrd/apply'

NOTE_DB_DIR  = N_("Directory where the RRDtool databases should be written.")
NOTE_RRDTOOL = N_("Path to the rrdtool binary. By default the server will look in the PATH.")


class Plugin_rrd (CTK.Plugin):
    def __init__ (self, key):
        CTK.Plugin.__init__ (self, key)

        # GUI
        table = CTK.PropsAuto (URL_APPLY)
        table.Add (_('Round Robin Database directory'), CTK.TextCfg('%s!database_dir'%(key), True), _(NOTE_DB_DIR))
        table.Add (_('Custom rrdtool binary'),  CTK.TextCfg('%s!rrdtool_path'%(key), True), _(NOTE_RRDTOOL))
        self += table

        # Input Validation
        VALS = [('%s!database_dir'%(key), validations.is_local_dir_exists),
                ('%s!rrdtool_path'%(key), validations.is_exec_file)]

        CTK.publish ('^%s'%(URL_APPLY), CTK.cfg_apply_post, validation=VALS, method="POST")
