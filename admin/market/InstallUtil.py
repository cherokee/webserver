# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2010 Alvaro Lopez Ortega
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
import sys
import SystemInfo

#
# User & Group management
#

def get_installation_UID():
    whoami = os.getuid()
    if sys.platform == 'linux2':
        return (str(whoami), '0')[whoami == 0]
    elif sys.platform == 'darwin':
        return (str(whoami), '0')[whoami == 0]

    # Solaris RBAC, TODO
    return (str(whoami), '0')[whoami == 0]

def get_installation_GID():
    groups     = os.getgroups()
    root_group = SystemInfo.get_info()['group_root']

    if sys.platform == 'linux2':
        return (str(groups[0]), root_group)[0 in groups]
    elif sys.platform == 'darwin':
        return (str(groups[0]), root_group)[0 in groups]

    # Solaris RBAC, TODO
    return (str(whoami), root_group)[whoami == 0]

def current_UID_is_admin():
    if sys.platform == 'linux2':
        return os.getuid() == 0
    elif sys.platform == 'darwin':
        return os.getuid() == 0

    # Other cases (such Solaris) may vary. By the moment, let's assume
    # 'root' is the only one who is admin.
    return os.getuid() == 0
