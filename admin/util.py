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
import sys
import glob
import socket
import subprocess

import CTK


#
# Deal with os.popen and subprocess issues in Python 2.4
#
def run (command, stdout=True, stderr=False, retcode=False):
    returns = 0

    args = {'shell': True, 'close_fds': True}
    if stdout:
        args['stdout'] = subprocess.PIPE
        returns += 1
    if stderr:
        args['stderr'] = subprocess.PIPE
        returns += 1
    if retcode:
        returns += 1

    p = subprocess.Popen (command, **args)

    if stdout:
        stdout_output = p.stdout.read()
    if stderr:
        stderr_output = p.stderr.read()

    if stdout:
        p.stdout.close()
    if stderr:
        p.stderr.close()

    ret = p.poll()

    if returns > 1:
        d = {}
        if stdout:
            d['stdout'] = stdout_output
        if stderr:
            d['stderr'] = stderr_output
        if retcode:
            d['retcode'] = ret
        return d

    if stdout:
        return stdout_output
    elif stderr:
        return stderr_output
    elif retcode:
        return ret

    assert False, "Should not happened"


#
# Strings
#
def bool_to_active (b):
    return (_('Inactive'), _('Active'))[bool(b)]

def bool_to_onoff (b):
    return (_('Off'), _('On'))[bool(b)]

def bool_to_yesno (b):
    return (_('No'), _('Yes'))[bool(b)]

def trans_options (options):
    """Translate the options with gettext"""
    return [(x[0], _(x[1])) for x in options]


#
# Virtual Server
#

def cfg_vsrv_get_next():
    """ Get the prefix of the next vserver """
    tmp = [int(x) for x in CTK.cfg.keys("vserver")]
    tmp.sort()
    next = str(tmp[-1] + 10)
    return "vserver!%s" % (next)

def cfg_vsrv_rule_get_next (pre):
    """ Get the prefix of the next rule of a vserver """
    tmp = [int(x) for x in CTK.cfg.keys("%s!rule"%(pre))]
    tmp.sort()
    if tmp:
        next = tmp[-1] + 100
    else:
        next = 100
    return (next, "%s!rule!%d" % (pre, next))

def cfg_vsrv_rule_find_extension (pre, extension):
    """Find an extension rule in a virtual server """
    for r in CTK.cfg.keys("%s!rule"%(pre)):
        p = "%s!rule!%s" % (pre, r)
        if CTK.cfg.get_val ("%s!match"%(p)) == "extensions":
            if extension in CTK.cfg.get_val ("%s!match!extensions"%(p)):
                return p

def cfg_vsrv_rule_find_regexp (pre, regexp):
    """Find a regular expresion rule in a virtual server """
    for r in CTK.cfg.keys("%s!rule"%(pre)):
        p = "%s!rule!%s" % (pre, r)
        if CTK.cfg.get_val ("%s!match"%(p)) == "request":
            if regexp == CTK.cfg.get_val ("%s!match!request"%(p)):
                return p

#
# Information Sources
#

def cfg_source_get_next ():
    tmp = [int(x) for x in CTK.cfg.keys("source")]
    if not tmp:
        return (1, "source!1")
    tmp.sort()
    next = tmp[-1] + 10
    return (next, "source!%d" % (next))

def cfg_source_find_interpreter (in_interpreter = None,
                                 in_nick        = None):
    for i in CTK.cfg.keys("source"):
        if CTK.cfg.get_val("source!%s!type"%(i)) != 'interpreter':
            continue

        if (in_interpreter and
            in_interpreter in CTK.cfg.get_val("source!%s!interpreter"%(i), '')):
            return "source!%s" % (i)

        if (in_nick and
            in_nick in CTK.cfg.get_val("source!%s!nick"%(i), '')):
            return "source!%s" % (i)

def cfg_source_find_empty_port (n_ports=1):
    ports = []
    for i in CTK.cfg.keys("source"):
        host = CTK.cfg.get_val ("source!%s!host"%(i))
        if not host: continue

        colon = host.rfind(':')
        if colon < 0: continue

        port = int (host[colon+1:])
        if port < 1024: continue

        ports.append (port)

    pport = 1025
    while pport+n_ports < 65535:
        pports = range(pport, pport + n_ports)
        for x in pports:
            if x in ports:
                pport += 1
                break
            return pport

    assert (False)

def cfg_source_find_free_port (host_name='localhost'):
    """Return a port not currently running anything"""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((host_name, 0))
    addr, port = s.getsockname()
    s.close()
    return port

def cfg_source_get_localhost_addr ():
    x, x, addrs = socket.gethostbyname_ex('localhost')
    if addrs:
        return addrs[0]
    return None

def cfg_get_surrounding_repls (macro, value, n_minus=9, n_plus=9):
    replacements = {}

    tmp = value.split('!')
    pre = '!'.join(tmp[:-1])
    num = int(tmp[-1])

    for n in range(n_minus):
        replacements['%s_minus%d'%(macro,n+1)] = '%s!%d' %(pre, num-(n+1))

    for n in range(n_plus):
        replacements['%s_plus%d'%(macro,n+1)] = '%s!%d' %(pre, num+(n+1))

    return replacements

#
# Version strings management
#

def version_to_int (v):
    num = 0
    tmp = v.split('.')

    if len(tmp) >= 3:
        num += int(tmp[2]) * (10**3)
    if len(tmp) >= 2:
        num += int(tmp[1]) * (10**6)
    if len(tmp) >= 1:
        num += int(tmp[0]) * (10**9)

    return num

def version_cmp (x, y):
    xp = x.split('b')
    yp = y.split('b')

    if len(xp) > 1:
        x_ver  = version_to_int(xp[0])
        x_beta = xp[1]
    else:
        x_ver  = version_to_int(xp[0])
        x_beta = None

    if len(yp) > 1:
        y_ver  = version_to_int(yp[0])
        y_beta = yp[1]
    else:
        y_ver  = version_to_int(yp[0])
        y_beta = None

    if x_ver == y_ver:
        if not x_beta and not y_beta: return 0
        if not y_beta: return -1
        if not x_beta: return  1
        return cmp(int(x_beta),int(y_beta))

    elif x_ver > y_ver:
        return 1

    return -1


#
# Paths
#

def path_find_binary (executable, extra_dirs=[], custom_test=None, default=None):
    """Find an executable.
    It checks 'extra_dirs' and the PATH.
    The 'executable' parameter can be either a string or a list.
    """

    assert (type(executable) in [str, list])

    # Extra dirs evaluation
    dirs = []
    for d in extra_dirs:
        if '*' in d or '?' in d:
            dirs += glob.glob (d)
        else:
            dirs.append(d)

    # $PATH
    env_path = os.getenv("PATH")
    if env_path:
        dirs += filter (lambda x: x, env_path.split(":"))

    # Check
    if type(executable) != list:
        executable = [executable]

    for exe in executable:
        for dir in dirs:
            fp = os.path.join (dir, exe)
            if os.path.exists (fp):
                if custom_test:
                    if not custom_test(fp):
                        continue
                return fp

    # Not found
    return default

def path_find_w_default (path_list, default=''):
    """Find a path.
    It checks a list of paths (that can contain wildcards),
    if none exists default is returned.
    """
    for path in path_list:
        if '*' in path or '?' in path:
            to_check = glob.glob (path)
        else:
            to_check = [path]
        for p in to_check:
            if os.path.exists (p):
                return p
    return default

def path_eval_exist (path_list):
    """Evaluate a list of potential paths.
    Returns the paths that actually exist.
    """
    exist = []
    for path in path_list:
        if '*' in path or '?' in path:
            to_check = glob.glob (path)
        else:
            to_check = [path]
        for p in to_check:
            if os.path.exists (p):
                exist.append (p)
    return exist



#
# OS
#
def os_get_document_root():
    if sys.platform == 'darwin':
        return "/Library/WebServer/Documents"
    elif sys.platform.startswith('linux'):
        if os.path.exists ("/etc/redhat-release"):
            return '/var/www'
        elif os.path.exists ("/etc/fedora-release"):
            return '/var/www'
        elif os.path.exists ("/etc/SuSE-release"):
            return '/srv/www/htdocs'
        elif os.path.exists ("/etc/debian_version"):
            return '/var/www'
        elif os.path.exists ("/etc/gentoo-release"):
            return '/var/www'
        elif os.path.exists ("/etc/slackware-version"):
            return '/var/www'
        return '/var/www'

    return ''


#
# Misc
#
def split_list (value):
    ids = []
    for t1 in value.split(','):
        for t2 in t1.split(' '):
            id = t2.strip()
            if not id:
                continue
            ids.append(id)
    return ids


def lists_differ (a, b):
    """Compare lists disregarding order"""
    if len(a) != len(b):
        return True
    if bool (set(a)-set(b)):
        return True
    if bool (set(b)-set(a)):
        return True
    return False


def get_real_path (name, nochroot=False):
    """Get real path accounting for chrooted environments"""
    chroot = CTK.cfg.get_val('server!chroot')
    if chroot and not nochroot:
        fullname = os.path.normpath (chroot + os.path.sep + name)
    else:
        fullname = name

    return fullname
