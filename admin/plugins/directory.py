# Cheroke Admin
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
import RuleSimple
import validations

NOTE_DIRECTORY = N_("Public Web Directory to which content the configuration will be applied.")

class Plugin_directory (RuleSimple.PluginSimple):
    def __init__ (self, key, **kwargs):
        RuleSimple.PluginSimple.__init__ (self, key, _('Web Directory'), "directory", _(NOTE_DIRECTORY), _('Directory'), validations.is_web_path, **kwargs)
