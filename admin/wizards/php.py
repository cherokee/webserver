# -*- coding: utf-8 -*-
#
# Cherokee-admin
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#      Taher Shihadeh <taher@unixwars.com>
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

import re
import os
import CTK
import Wizard
import validations
import popen
import SystemInfo

from util import *

NOTE_WELCOME_H1 = N_("Welcome to the PHP Wizard")
NOTE_WELCOME_P1 = N_('<a target="_blank" href="http://php.net/">PHP</a> is a widely-used general-purpose scripting language that is especially suited for Web development and can be embedded into HTML.')

NOTE_LOCAL_H1   = N_("Document Root")
NOTE_LOCAL_DIR  = N_("Local directory that will contain the web documents. Example: /var/www.")

NOTE_HOST_H1    = N_("New Virtual Server Details")
NOTE_HOST       = N_("Host name of the virtual server that is about to be created.")

NOTE_NOT_FOUND_H1 = N_("PHP Interpreter Not Found")
NOTE_NOT_FOUND    = N_("A valid PHP interpreter could not be found in your system.")
NOTE_NOT_FOUND2   = N_("Please, check out Cherokee's PHP Documentation to learn how to set it up.")

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
                 '/usr/php/*.*/bin',
                 '/usr/pkg/libexec/cgi-bin',
                 # FPM
                 '/usr/sbin',
                 '/usr/local/sbin',
                 '/opt/local/sbin',
                 '/usr/gnu/sbin']

FPM_ETC_PATHS = ['/etc/php*/fpm/*.conf',
                 '/etc/php*/fpm/*.d/*',
                 '/etc/php*-fpm.d/*',
                 # Old php-fpm
                 '/etc/php*/fpm/php*fpm.conf',
                 '/usr/local/etc/php*fpm.conf',
                 '/opt/php*/etc/php*fpm.conf',
                 '/opt/local/etc/php*/php*fpm.conf',
                 '/etc/php*/*/php*fpm.conf',
                 '/etc/php*/php*fpm.conf']

STD_ETC_PATHS = ['/etc/php.ini',
                 '/etc/php.d/*',
                 '/etc/php*/cgi/php.ini',
                 '/usr/local/etc/php.ini',
                 '/opt/php*/etc/php.ini',
                 '/opt/local/etc/php*/php.ini',
                 '/etc/php*/*/php.ini',
                 '/etc/php*/php.ini',
                 '/usr/local/lib*/php.ini']

CFG_PREFIX    = 'tmp!wizard!php'


#
# Public
#
def check_php_interpreter():
    # PHP Source
    source = __find_source()
    if source:
        return True

    # PHP binary
    php_path = path_find_binary (DEFAULT_BINS,
                                 extra_dirs  = DEFAULT_PATHS,
                                 custom_test = __test_php_fcgi)
    if php_path:
        return True

    # No PHP
    return False


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

        # Add the Source
        php_bin = php_path.split('/')[-1]

        try:
            if php_bin not in FPM_BINS:
                ret = __source_add_std (php_path)
            else:
                ret = __source_add_fpm (php_path)
        except Exception, e:
            return str(e)

        source = __find_source()

    # Figure the timeout limit
    interpreter = CTK.cfg.get_val ('%s!interpreter'%(source))
    timeout     = CTK.cfg.get_val ('%s!timeout'    %(source))

    if not timeout:
        if not interpreter:
            timeout = PHP_DEFAULT_TIMEOUT
        elif 'fpm' in interpreter:
            timeout = __figure_fpm_settings()['timeout']
        else:
            timeout = __figure_std_settings()['timeout']

    # Add a new Extension PHP rule
    if not rule:
        next = CTK.cfg.get_next_entry_prefix('%s!rule'%(key))
        src_num = source.split('!')[-1]

        CTK.cfg['%s!match' %(next)]                     = 'extensions'
        CTK.cfg['%s!match!extensions' %(next)]          = 'php'
        CTK.cfg['%s!match!check_local_file' %(next)]    = '1'
        CTK.cfg['%s!match!final' %(next)]               = '0'
        CTK.cfg['%s!handler' %(next)]                   = 'fcgi'
        CTK.cfg['%s!handler!balancer' %(next)]          = 'round_robin'
        CTK.cfg['%s!handler!balancer!source!1' %(next)] = src_num
        CTK.cfg['%s!handler!error_handler' %(next)]     = '1'
        CTK.cfg['%s!encoder!gzip' %(next)]              = '1'
        CTK.cfg['%s!timeout' %(next)]                   = timeout

        # Front-Line Cache
        CTK.cfg['%s!flcache' %(next)]                   = 'allow'
        CTK.cfg['%s!flcache!policy' %(next)]            = 'explicitly_allowed'

        # GZip encoder
        CTK.cfg['%s!encoder!gzip' %(next)]              = 'allow'

    # Index files
    indexes = filter (None, CTK.cfg.get_val ('%s!directory_index' %(key), '').split(','))
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


def figure_modules():
    """Return list of modules available to PHP"""
    php_path = path_find_binary (DEFAULT_BINS,
                                 extra_dirs  = DEFAULT_PATHS,
                                 custom_test = __test_php_fcgi)

    ret = popen.popen_sync ('%s -m' %(php_path))
    modules = re.findall('(^[a-zA-Z0-9].*$)', ret['stdout'], re.MULTILINE)

    return modules


def figure_php_information():
    """Parse PHP settings into a dictionary"""
    php_path = path_find_binary (DEFAULT_BINS,
                                 extra_dirs  = DEFAULT_PATHS,
                                 custom_test = __test_php_fcgi)

    ret = popen.popen_sync ('%s -i' %(php_path))

    # Output can either be in HTML format or as a PHP array.
    if 'Content-type: text/html' in ret['stdout']:
        regex = """^<tr><td class="e">(.*?)</td><td class="v">(.*?)</td></tr>$"""
    else:
        regex = """^(.*?) => (.*?)( => .*)?$"""

    settings = {}
    tags     = re.compile(r'<.*?>')
    matches  = re.findall(regex, ret['stdout'], re.MULTILINE)

    for setting in matches:
        key, val = setting[0].strip(), setting[1].strip()
        val = tags.sub('', val)
        settings[key] = val

    return settings


def figure_php_user (pre_vsrv):
    """Determine PHP user/group accounting for config file, source UID, and server UID"""

    server_user  = CTK.cfg.get_val ('server!user',  str(os.getuid()))
    server_group = CTK.cfg.get_val ('server!group', str(os.getgid()))
    php_info     = get_info (pre_vsrv)
    interpreter  = CTK.cfg.get_val ('%s!interpreter' %(php_info['source']), '')

    if 'fpm' in interpreter:
        fpm_conf  = _figure_fpm_settings()
        php_user  = fpm_conf.get('user',  server_user)
        php_group = fpm_conf.get('group', server_group)
    else:
        php_user  = CTK.cfg.get_val ('%s!user'  %(php_info['source']), server_user)
        php_group = CTK.cfg.get_val ('%s!group' %(php_info['source']), server_group)

    return  {'php_user':  php_user,
             'php_group': php_group}


#
# Public URLs
#

URL_WIZARD_RULE_R  = r'^/wizard/vserver/(\d+)/php$'
URL_WIZARD_APPLY   = '/wizard/vserver/%s/php/apply'
URL_WIZARD_APPLY_R = r'^/wizard/vserver/(\d+)/php/apply'
URL_APPLY          = '/wizard/vserver/php/apply'

class Commit:
    def Commit_VServer (self):
        # Create the new Virtual Server
        next = CTK.cfg.get_next_entry_prefix('vserver')

        CTK.cfg['%s!nick'%(next)]            = CTK.cfg.get_val('%s!host'%(CFG_PREFIX))
        CTK.cfg['%s!document_root'%(next)]   = CTK.cfg.get_val('%s!droot'%(CFG_PREFIX))
        CTK.cfg['%s!directory_index'%(next)] = 'index.php,index.html'
        CTK.cfg['%s!rule!1!match'%(next)]    = 'default'
        CTK.cfg['%s!rule!1!handler'%(next)]  = 'common'

        Wizard.CloneLogsCfg_Apply ('%s!logs_as_vsrv'%(CFG_PREFIX), next)

        # PHP
        error = wizard_php_add (next)
        if error:
            del CTK.cfg['vserver!%s'%(next)]
            return {'ret': 'error', 'errors': {'msg': error}}

        # Clean up
        CTK.cfg.normalize ('%s!rule'%(next))
        CTK.cfg.normalize ('vserver')

        del (CTK.cfg[CFG_PREFIX])
        return CTK.cfg_reply_ajax_ok()


    def Commit_Rule (self):
        vserver = CTK.cfg.get_val ('%s!vsrv_num'%(CFG_PREFIX))

        error = wizard_php_add ('vserver!%s'%(vserver))
        if error:
            return {'ret': 'error', 'errors': {'msg': error}}

        return CTK.cfg_reply_ajax_ok()


    def __call__ (self):
        if CTK.post.pop('final'):
            # Apply POST
            CTK.cfg_apply_post()

            # VServer or Rule?
            if CTK.cfg.get_val ('%s!vsrv_num'%(CFG_PREFIX)):
                return self.Commit_Rule()
            return self.Commit_VServer()

        return CTK.cfg_apply_post()


class Host:
    def __call__ (self):
        table = CTK.PropsTable()
        table.Add (_('New Host Name'),    CTK.TextCfg ('%s!host'%(CFG_PREFIX), False, {'value': 'www.example.com', 'class': 'noauto'}), _(NOTE_HOST))
        table.Add (_('Use Same Logs as'), Wizard.CloneLogsCfg('%s!logs_as_vsrv'%(CFG_PREFIX)), _(Wizard.CloneLogsCfg.NOTE))

        notice = CTK.Notice('error', props={'class': 'no-see'})

        submit = CTK.Submitter (URL_APPLY)
        submit += CTK.Hidden('final', '1')
        submit += table
        submit.bind ('submit_fail', "$('#%s').show().html(event.ret_data.errors.msg);"%(notice.id))

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_HOST_H1)))
        cont += submit
        cont += notice
        cont += CTK.DruidButtonsPanel_PrevCreate_Auto()
        return cont.Render().toStr()


class DocumentRoot:
    def __call__ (self):
        table = CTK.PropsTable()
        table.Add (_('Document Root'), CTK.TextCfg ('%s!droot'%(CFG_PREFIX), False, {'value': '/var/www'}), _(NOTE_LOCAL_DIR))

        submit = CTK.Submitter (URL_APPLY)
        submit += table

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_LOCAL_H1)))
        cont += submit
        cont += CTK.DruidButtonsPanel_PrevNext_Auto()
        return cont.Render().toStr()


class WelcomeVserver:
    def __call__ (self):
        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('php', {'class': 'wizard-descr'})

        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += Wizard.CookBookBox ('cookbook_php')
        cont += box
        cont += CTK.DruidButtonsPanel_Next_Auto()

        return cont.Render().toStr()


class WelcomeRule:
    def __call__ (self):
        vserver = re.findall (URL_WIZARD_RULE_R, CTK.request.url)[0]

        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_WELCOME_H1)))
        cont += Wizard.Icon ('php', {'class': 'wizard-descr'})

        notice = CTK.Notice('error', props={'class': 'no-see'})

        box = CTK.Box ({'class': 'wizard-welcome'})
        box += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_WELCOME_P1)))
        box += Wizard.CookBookBox ('cookbook_php')
        box += notice

        submit = CTK.Submitter (URL_WIZARD_APPLY %(vserver))
        submit += CTK.Hidden ('final', '1')

        vsrv_num = re.findall (r'^/wizard/vserver/(\d+)/', CTK.request.url)[0]
        submit += CTK.Hidden('%s!vsrv_num'%(CFG_PREFIX), vsrv_num)

        submit += box
        submit.bind ('submit_fail', "$('#%s').show().html(event.ret_data.errors.msg);"%(notice.id))

        cont += submit
        cont += CTK.DruidButtonsPanel_Create()

        return cont.Render().toStr()

VALS = [
    ('%s!host'  %(CFG_PREFIX), validations.is_not_empty),
    ('%s!host'  %(CFG_PREFIX), validations.is_new_vserver_nick),
    ('%s!droot' %(CFG_PREFIX), validations.is_not_empty),
    ('%s!droot' %(CFG_PREFIX), validations.is_local_dir_exists),
]

# Rule
CTK.publish (URL_WIZARD_RULE_R,  WelcomeRule)
CTK.publish (URL_WIZARD_APPLY_R, Commit, method="POST")

# VServer
CTK.publish ('^/wizard/vserver/php$',   WelcomeVserver)
CTK.publish ('^/wizard/vserver/php/2$', DocumentRoot)
CTK.publish ('^/wizard/vserver/php/3$', Host)
CTK.publish (r'^%s'%(URL_APPLY),        Commit, method="POST", validation=VALS)


#
# External Stages
#

def External_FindPHP():
    # Add PHP if needed
    have_php = check_php_interpreter()
    if not have_php:
        cont = CTK.Container()
        cont += CTK.RawHTML ('<h2>%s</h2>' %(_(NOTE_NOT_FOUND_H1)))
        cont += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_NOT_FOUND)))
        cont += CTK.RawHTML ('<p>%s</p>' %(_(NOTE_NOT_FOUND2)))
        cont += CTK.DruidButtonsPanel_Cancel()
        return cont.Render().toStr()

    return CTK.DruidContent_TriggerNext().Render().toStr()


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


def __figure_std_settings():
    # Find config file
    paths = []
    for p in STD_ETC_PATHS:
        paths.append (p)
        paths.append ('%s-*' %(p))

    std_info = {}

    # For each configuration file
    for conf_file in path_eval_exist (paths):
        # Read
        try:
            content = open (conf_file, 'r').read()
        except:
            continue

        # Timeout
        if not std_info.get('timeout'):
            tmp = re.findall (r'^max_execution_time\s*=\s*(\d*)', content)
            if tmp:
                std_info['timeout'] = tmp[0]

            # Config file
            if not std_info.get('conf_file'):
                std_info['conf_file'] = conf_file

    # Set last minute defaults
    if not std_info.get('timeout'):
        std_info['timeout'] = PHP_DEFAULT_TIMEOUT

    if not std_info.get('listen'):
        std_info['listen'] = cfg_source_get_localhost_addr()

    return std_info


def __figure_fpm_settings():
    fpm_info = {}

    # Find config file
    paths = []
    for p in FPM_ETC_PATHS:
        paths.append (p)
        paths.append ('%s-*' %(p))

    # For each configuration file
    suitable_confs = path_eval_exist (paths)
    if not suitable_confs:
        return {}

    # Read the configuration files
    for conf_file in suitable_confs:
        # Read
        try:
            content = open (conf_file, 'r').read()
        except:
            continue

        # Listen
        if not fpm_info.get('listen'):
            tmp = re.findall (r'<value name="listen_address">(.*?)</value>', content)
            if tmp:
                fpm_info['listen'] = tmp[0]
            else:
                tmp = re.findall (r'^listen *= *(.+)$', content, re.M)
                if tmp:
                    fpm_info['listen']  = tmp[0]

        # Timeout
        if not fpm_info.get('timeout'):
            tmp = re.findall (r'<value name="request_terminate_timeout">(\d*)s*</value>', content)
            if tmp:
                fpm_info['timeout'] = tmp[0]
            else:
                tmp = re.findall (r'^request_terminate_timeout *= *(\d*)s*', content, re.M)
                if tmp:
                    fpm_info['timeout'] = tmp[0]

        # Filename
        if not fpm_info.get('conf_file'):
            if '.conf' in conf_file:
                fpm_info['conf_file'] = conf_file

        # User
        if not fpm_info.get('user'):
            tmp = re.findall (r'<value name="user">(.*?)</value>', content)
            if tmp:
                fpm_info['user'] = tmp[0]
            else:
                tmp = re.findall (r'^user *= *(.*)$', content, re.M)
                if tmp:
                    fpm_info['user'] = tmp[0]

        # Group
        if not fpm_info.get('group'):
            tmp = re.findall (r'<value name="group">(.*?)</value>', content)
            if tmp:
                fpm_info['group'] = tmp[0]
            else:
                tmp = re.findall (r'^group *= *(.*)$', content, re.M)
                if tmp:
                    fpm_info['group'] = tmp[0]

    # Set last minute defaults
    if not fpm_info.get('timeout'):
         fpm_info['timeout'] = PHP_DEFAULT_TIMEOUT

    if not fpm_info.get('listen'):
         fpm_info['listen'] = "127.0.0.1"

    return fpm_info


def __source_add_std (php_path):
    # Read settings
    std_info = __figure_std_settings()
    if not std_info:
        raise Exception (_('Could not determine PHP-CGI settings.'))

    if not std_info.has_key('conf_file'):
        raise Exception (_('Could not determine PHP-CGI configuration file.'))

    # IANA: TCP ports 47809-47999 are unassigned
    TCP_PORT = 47990
    host     = std_info['listen']

    # Add the Source
    next = CTK.cfg.get_next_entry_prefix('source')

    CTK.cfg['%s!nick'          %(next)] = 'PHP Interpreter'
    CTK.cfg['%s!type'          %(next)] = 'interpreter'
    CTK.cfg['%s!host'          %(next)] = '%(host)s:%(TCP_PORT)d' %(locals())
    CTK.cfg['%s!interpreter'   %(next)] = '%(php_path)s -b %(host)s:%(TCP_PORT)d' %(locals())
    CTK.cfg['%s!env_inherited' %(next)] = '0'

    CTK.cfg['%s!env!PHP_FCGI_MAX_REQUESTS' %(next)] = SAFE_PHP_FCGI_MAX_REQUESTS
    CTK.cfg['%s!env!PHP_FCGI_CHILDREN'     %(next)] = '5'

    return next

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
    if sys.platform.startswith('linux'):
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

def __source_add_fpm (php_path):
    # Read settings
    fpm_info = __figure_fpm_settings()
    if not fpm_info:
        raise Exception (_('Could not determine PHP-fpm settings.'))

    listen    = fpm_info['listen']
    conf_file = fpm_info['conf_file']

    # Add Source
    next = CTK.cfg.get_next_entry_prefix('source')

    CTK.cfg['%s!nick'        %(next)] = 'PHP Interpreter'
    CTK.cfg['%s!type'        %(next)] = 'interpreter'
    CTK.cfg['%s!host'        %(next)] = listen
    CTK.cfg['%s!interpreter' %(next)] = '%(php_path)s --fpm-config %(conf_file)s' %(locals())

    # In case FPM has specific UID/GID and differs from Cherokee's,
    # the interpreter must by spawned by root.
    #
    server_user  = CTK.cfg.get_val ('server!user',  str(os.getuid()))
    server_group = CTK.cfg.get_val ('server!group', str(os.getgid()))

    root_user    = get_installation_UID()
    root_group   = get_installation_GID()
    php_user     = fpm_info.get('user',  server_user)
    php_group    = fpm_info.get('group', server_group)

    if php_user != server_user or php_group != server_group:
        CTK.cfg['%s!user'  %(next)] = root_user
        CTK.cfg['%s!group' %(next)] = root_group

    return next


def __test_php_fcgi (path):
    f = os.popen('%s -v' %(path), 'r')
    output = f.read()
    try: f.close()
    except: pass
    return 'fcgi' in output
