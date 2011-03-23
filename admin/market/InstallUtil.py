# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2001-2011 Alvaro Lopez Ortega
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
import re
import pwd
import sys
import SystemInfo

#
# User & Group management
#

def get_installation_UID():
    whoami = os.getuid()

    try:
        info = pwd.getpwuid (whoami)
        return info.pw_name
    except:
        return str(whoami)


def get_installation_GID():
    root_group = SystemInfo.get_info()['group_root']

    try:
        groups = os.getgroups()
        groups.sort()
        first_group = str(groups[0])
    except OSError,e:
        # os.getgroups can fail when run as root (MacOS X 10.6)
        if os.getuid() != 0:
            raise e
        first_group = str(root_group)

    # Systems
    if sys.platform == 'linux2':
        if os.getuid() == 0:
            return root_group
        return first_group
    elif sys.platform == 'darwin':
        if os.getuid() == 0:
            return root_group
        return first_group

    # Solaris RBAC, TODO
    if os.getuid() == 0:
        return root_group
    return first_group


def current_UID_is_admin():
    if sys.platform == 'linux2':
        return os.getuid() == 0
    elif sys.platform == 'darwin':
        return os.getuid() == 0

    # Other cases (such Solaris) may vary. By the moment, let's assume
    # 'root' is the only one who is admin.
    return os.getuid() == 0

#
# Macro replacement
#

def replacement_cmd (command, replacement_dict):
    assert type(replacement_dict) == dict
    assert type(command) == str

    macros = re.findall ('\${(.*?)}', command)
    for macro in macros:
        command = command.replace('${%s}'%(macro), replacement_dict.get(macro, '${%s}'%(macro)))

    return command
