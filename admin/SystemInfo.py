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
import re
import sys
import struct
import util
import subprocess

LINUX_DISTS_ETC = [('gentoo-release',     'gentoo'),
                   ('fedora-release',     'fedora'),
                   ('turbolinux-release', 'turbolinux'),
                   ('mandriva-release',   'mandriva'),
                   ('SuSE-release',       'suse'),
                   ('knoppix-version',    'knoppix'),
                   ('slackware-version',  'slackware'),
                   ('slackware-release',  'slackware'),
                   ('redflag-release',    'redflag'),
                   ('conectiva-release',  'conectiva'),
                   ('redhat-release',     'redhat'), # rhel/centos
                   ('debian_version',     'debian'),
                   ]


_cache = None
def get_info():
    global _cache
    if not _cache:
        _cache = build_info()
    return _cache


def build_info():
    info = {}

    # Uname
    tmp = os.uname()
    info['system']  = tmp[0]
    info['node']    = tmp[1]
    info['release'] = tmp[2]
    info['version'] = tmp[3]
    info['machine'] = tmp[4]

    # Python version
    tmp = re.findall('([\w.+]+)\s*\((.+),\s*([\w ]+),\s*([\w :]+)\)\s*\[([^\]]+)\]?', sys.version)[0]

    #  2.4.6 (#1, Oct 15 2009, 11:10:21) \n[GCC 4.2.1 (Apple Inc. build 5646)]
    #  2.6.5 (r265:79063, Apr 16 2010, 13:57:41) \n[GCC 4.4.3]

    info['python_version']    = tmp[0]
    info['python_buildno']    = tmp[1]
    info['python_builddate']  = tmp[2]
    info['python_buildtime']  = tmp[3]
    info['python_compiler']   = tmp[4]
    info['python_executable'] = sys.executable

    # Architecture bits
    size = struct.calcsize('P')
    info['architecture'] = str(size*8) + 'bit'

    # Linux distribution
    if info['system'] == 'Linux':
        _figure_linux_info (info)
    elif info['system'] == 'Darwin':
        _figure_macos_info (info)
    elif info['system'] == 'FreeBSD':
        _figure_freebsd_info (info)
    elif info['system'] == 'SunOS':
        _figure_solaris_info (info)

    # Users and Groups
    if os.access ('/etc/group', os.R_OK):
        group = open('/etc/group', 'r').read()
        if re.findall (r'^root:', group, re.MULTILINE):
            info['group_root'] = 'root'
        elif re.findall (r'^wheel:', group, re.MULTILINE):
            info['group_root'] = 'wheel'

    if not info.get ('group_root'):
        info['group_root'] = '0'

    return info


def _figure_linux_info (info):
    # Try to figure out the distro
    for dp in LINUX_DISTS_ETC:
        if os.path.exists (os.path.join ('/etc', dp[0])):
            info['linux_distro_id'] = dp[1]

            # Distinguish RedHat from CentOS
            if dp[1] == 'redhat':
                release = open (os.path.join ('/etc', dp[0])).read()
                info['linux_distro_description'] = release
                regex = '(.*?) release (.*)'
                tmp = re.findall(regex, release)
                if tmp:
                    info['linux_distro_id'] = tmp[0][0].lower()
                    info['linux_distro_release'] = tmp[0][1].lower()

            # Populate version in case it is Debian based
            elif dp[1] == 'debian':
                release = open (os.path.join ('/etc', dp[0])).read().replace('\n','')
                info['linux_distro_release'] = release
                info['linux_distro_description'] = 'Debian %s' %(release)

    # Check the LSB release file (do this last so LSB prevails in case of conflict)
    if os.path.exists('/etc/lsb-release'):
        release = open('/etc/lsb-release','r').read()

        for key in ('id', 'release', 'description'):
            regex = 'DISTRIB_%s\=(.*)' %(key.upper())
            tmp = re.findall(regex, release)
            if tmp:
                info['linux_distro_%s'%(key)] = tmp[0]


def _figure_macos_info (info):
    port_bin = util.path_find_binary ("port", ['/opt/local/bin'])
    info['macports']      = bool(port_bin)
    info['macports_bin']  = port_bin
    if port_bin:
        info['macports_path'] = '/'.join (port_bin.split('/')[:-1])


def _figure_freebsd_info (info):
    path = '/usr/ports'
    if os.path.isdir (path):
        info['freebsd_ports_path'] = path


def _figure_solaris_info (info):
    None


if __name__ == '__main__':
    import pprint
    pprint.pprint (get_info())
