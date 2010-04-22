# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#      Taher Shihadeh <taher@unixwars.com>
#
# Copyright (C) 2010 Alvaro Lopez Ortega
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

import re
import CTK
import Wizard

from util import *

NOTE_WELCOME_H1 = N_("Welcome to the PHP Wizard")
NOTE_WELCOME_P1 = N_('<a target="_blank" href="http://php.net/">PHP</a> is a widely-used general-purpose scripting language that is especially suited for Web development and can be embedded into HTML.')

PHP_DEFAULT_TIMEOUT        = '30'
SAFE_PHP_FCGI_MAX_REQUESTS = '490'

FPM_BINS = ['php-fpm', 'php5-fpm']
STD_BINS = ['php-cgi', 'php']

DEFAULT_BINS  = FPM_BINS + STD_BINS

DEFAULT_PATHS = ['/usr/bin',
                 '/opt/php',
                 '/usr/php/bin',
                 '/usr/sfw/bin',
                 '/usr/gnu/bin',
                 '/usr/local/bin',
                 '/opt/local/bin',
                 '/usr/pkg/libexec/cgi-bin']

FPM_ETC_PATHS = ['/etc/php*/fpm/php*fpm.conf',
                 '/usr/local/etc/php*fpm.conf',
                 '/opt/php*/etc/php*fpm.conf',
                 '/opt/local/etc/php*/php*fpm.conf',
                 '/etc/php*/*/php*fpm.conf']

STD_ETC_PATHS = ['/etc/php*/cgi/php.ini',
                 '/usr/local/etc/php.ini',
                 '/opt/php*/etc/php.ini',
                 '/opt/local/etc/php*/php.ini',
                 '/etc/php*/*/php.ini']

#
# Public
#
def wizard_php_add (key):
    # Sanity check
    if not CTK.cfg[key]:
        return _('Invalid Virtual Server reference.')

    # Gather information
    source = __find_source()
    rule   = __find_rule(key)

    # Add source if needed
    if not source:
        php_path = path_find_binary (DEFAULT_BINS,
                                     extra_dirs  = DEFAULT_PATHS,
                                     custom_test = __test_php_fcgi)
        if not php_path:
            return _('Could not find a suitable PHP interpreter.')

        # Check PHP type
        php_bin = php_path.split('/')[-1]
        if php_bin not in FPM_BINS:
            ret = __source_add_std (php_path)
        else:
            settings = __figure_fpm_settings()
            if not settings:
                return _('Could not determine PHP-fpm settings.')
            ret = __source_add_fpm (php_path)

        if not ret:
            return _('Could not determine correct interpreter settings.')

        source = __find_source()

    # Figure the timeout limit
    interpreter = CTK.cfg['%s!interpreter' %(source)]
    if interpreter and 'fpm' in interpreter:
        settings = __figure_fpm_settings()
        timeout  = settings['fpm_terminate_timeout']
    else:
        timeout = __figure_max_execution_time()

    # Add a new Extension PHP rule
    if not rule:
        next = CTK.cfg.get_next_entry_prefix('%s!rule'%(key))
        src_num = source.split('!')[-1]

        CTK.cfg['%s!match' %(next)]                     = 'extensions'
        CTK.cfg['%s!match!extensions' %(next)]          = 'php'
        CTK.cfg['%s!match!final' %(next)]               = '0'
        CTK.cfg['%s!handler' %(next)]                   = 'fcgi'
        CTK.cfg['%s!handler!balancer' %(next)]          = 'round_robin'
        CTK.cfg['%s!handler!balancer!source!1' %(next)] = src_num
        CTK.cfg['%s!handler!error_handler' %(next)]     = '1'
        CTK.cfg['%s!encoder!gzip' %(next)]              = '1'
        CTK.cfg['%s!timeout' %(next)]                   = timeout

    # Index files
    indexes = CTK.cfg.get_val ('%s!directory_index' %(key), '').split(',')
    if not 'index.php' in indexes:
        indexes.append ('index.php')
        CTK.cfg['%s!directory_index' %(key)] = ','.join(indexes)

    # Normalization
    CTK.cfg.normalize('%s!rule'%(key))


def get_info (key):
    rule   = __find_rule (key)
    source = __find_source ()

    rule_num = rule.split('!')[-1]

    return {'source': source, # cfg path
            'rule':   rule}   # cfg path


#
# Public URLs
#

URL_WIZARD_RULE_R  = r'^/wizard/vserver/(\d+)/php$'
URL_WIZARD_APPLY   = '/wizard/vserver/%s/php/apply'
URL_WIZARD_APPLY_R = r'^/wizard/vserver/(\d+)/php/apply$'

def Commit():
    vserver = re.findall (URL_WIZARD_APPLY_R, CTK.request.url)[0]
    error   = wizard_php_add ('vserver!%s'%(vserver))
    if error:
        return {'ret': 'error', 'errors': {'msg': error}}
    return CTK.cfg_reply_ajax_ok()

class Welcome:
    def __call__ (self):
        vserver = re.findall (URL_WIZARD_RULE_R, CTK.request.url)[0]

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('php', {'class': 'wizard-descr'})

        notice = CTK.Notice('error', props={'class': 'no-see'})

        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += notice

        submit = CTK.Submitter (URL_WIZARD_APPLY %(vserver))
        submit += CTK.Hidden ('aint', 'empty')
        submit += box
        submit.bind ('submit_fail', "$('#%s').show().html(event.ret_data.errors.msg);"%(notice.id))

        cont += submit
        cont += CTK.DruidButtonsPanel_Create()

        return cont.Render().toStr()

CTK.publish (URL_WIZARD_RULE_R,  Welcome)
CTK.publish (URL_WIZARD_APPLY_R, Commit, method="POST")


#
# Private
#

def __find_source():
    for binary in FPM_BINS + STD_BINS:
        source = cfg_source_find_interpreter (binary)
        if source:
            return source

def __find_rule (key):
    return cfg_vsrv_rule_find_extension (key, 'php')


def __figure_max_execution_time ():
    # Figure out the php.ini path
    paths = []
    for p in STD_ETC_PATHS:
        paths.append (p)
        paths.append ('%s-*' %(p))

    phpini_path = path_find_w_default (paths, None)
    if not phpini_path:
        return PHP_DEFAULT_TIMEOUT

    # Read the file
    try:
        content = open(phpini_path, 'r').read()
    except IOError:
        return PHP_DEFAULT_TIMEOUT

    # Try to read the max_execution_time
    tmp = re.findall (r'max_execution_time\s*=\s*(\d*)', content)
    if not tmp:
        return PHP_DEFAULT_TIMEOUT

    return tmp[0]

def __figure_fpm_settings():
    # Find config file
    paths = []
    for p in FPM_ETC_PATHS:
        paths.append (p)
        paths.append ('%s-*' %(p))

    fpm_conf = path_find_w_default (paths, None)
    if not fpm_conf:
        return None

    # Read
    try:
        content = open(fpm_conf, 'r').read()
    except IOError:
        return None

    # Extract info
    tmp = re.findall (r'<value name="listen_address">(.*?)</value>', content)
    if tmp:
        listen_address = tmp[0]

    tmp = re.findall (r'<value name="request_terminate_timeout">(\d*)s*</value>', content)
    if tmp:
        timeout = tmp[0]
    else:
        timeout = PHP_DEFAULT_TIMEOUT

    # Done
    return {'fpm_conf' :           fpm_conf,
            'fpm_listen_address' : listen_address,
            'fpm_terminate_timeout' : timeout}

def __source_add_std (php_path):
    # IANA: TCP ports 47809-47999 are unassigned
    TCP_PORT = 47990

    tcp_addr = cfg_source_get_localhost_addr()
    if not tcp_addr:
        return None

    next = CTK.cfg.get_next_entry_prefix('source')
    CTK.cfg['%s!nick' %(next)]        = 'PHP Interpreter'
    CTK.cfg['%s!type' %(next)]        = 'interpreter'
    CTK.cfg['%s!interpreter' %(next)] = '%s -b %s:%d' % (php_path, tcp_addr, TCP_PORT)
    CTK.cfg['%s!host' %(next)]        = '%s:%d' % (tcp_addr, TCP_PORT)
    CTK.cfg['%s!env!PHP_FCGI_MAX_REQUESTS' %(next)] = SAFE_PHP_FCGI_MAX_REQUESTS
    CTK.cfg['%s!env!PHP_FCGI_CHILDREN' %(next)]     = '5'
    return next

def __source_add_fpm (php_path):
    settings = __figure_fpm_settings()
    host = settings['fpm_listen_address']
    if not host:
        return None

    next = CTK.cfg.get_next_entry_prefix('source')
    CTK.cfg['%s!nick' %(next)]        = 'PHP Interpreter'
    CTK.cfg['%s!type' %(next)]        = 'interpreter'
    CTK.cfg['%s!interpreter' %(next)] = '%s --fpm-config %s' % (php_path, settings['fpm_conf'])
    CTK.cfg['%s!host' %(next)]        = host
    return next

def __test_php_fcgi (path):
    f = os.popen('%s -v' %(path), 'r')
    output = f.read()
    try: f.close()
    except: pass
    return '(cgi-fcgi)' in output
