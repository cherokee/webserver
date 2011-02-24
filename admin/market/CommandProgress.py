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
import CTK
import popen
import traceback
import threading
import SystemInfo
import Install_Log
import InstallUtil

MORE  = N_("Show error details")
LESS  = N_("Hide error details")
DELAY = 500 #ms

def error_to_HTML (error):
    # error = error.replace ('\n', '<br/>')
    error = error.replace ('\x07', '')
    error = error.replace ('\r', '')
    error = error.replace ('', '')
    return CTK.escape_html (error)


class Replacement_Dict:
    def __init__ (self):
        self.keys = {}

    def __setitem__ (self, key, func):
        self.keys[key] = func

    def replace (self, txt):
        for k in self.keys:
            macro = '${%s}'%(k)
            if macro in txt:
                val = self.keys[k]
                if hasattr (val, '__call__'):
                    replacement = val()
                else:
                    replacement = val
                txt = txt.replace (macro, replacement)
        return txt


class Replacement_Commons (Replacement_Dict):
    def __init__ (self):
        Replacement_Dict.__init__ (self)

        # Install user (root, whell, admin, etc)
        root_user  = InstallUtil.get_installation_UID()
        root_group = InstallUtil.get_installation_GID()

        # Server user (www-data, server, nobody, etc)
        #
        # Unusual case: getgroups() might return []. We try to cover
        # the corner case by assuming a safety value of [0] (because
        # we haven't any other better option, actually).
        groups = os.getgroups() or [0]
        groups.sort()

        server_user  = CTK.cfg.get_val ('server!user',  str(os.getuid()))
        server_group = CTK.cfg.get_val ('server!group', str(groups[0]))

        # Directories
        app_root = CTK.cfg.get_val ('tmp!market!install!root')

        # Replacements
        self['web_user']   = server_user
        self['web_group']  = server_group
        self['root_user']  = root_user
        self['root_group'] = root_group
        self['app_root']   = app_root


class CommandProgress (CTK.Box, Replacement_Commons):
    class Exec (CTK.Container):
        def __init__ (self, command_progress):
            CTK.Container.__init__ (self)

            # Length and current
            commands_len = len(command_progress.commands)
            if command_progress.error or \
               command_progress.executed >= commands_len:
                command_entry = command_progress.commands [command_progress.executed - 1]
            else:
                command_entry = command_progress.commands [command_progress.executed]

            # Title
            if command_entry.has_key('description'):
                title = CTK.escape_html (command_entry['description'])
            elif command_entry.has_key('command'):
                title = CTK.escape_html (command_entry['command'])
            elif command_entry.has_key('function'):
                title = CTK.escape_html (command_entry['function'].__name__)
            else:
                assert False, 'Unknown command entry type'

            # Error
            if command_progress.error:
                ret     = command_progress.last_popen_ret
                percent = command_progress.executed * 100.0 / (commands_len + 1)

                if ret.get('command'):
                    title_error = CTK.escape_html (ret['command'])
                elif command_entry.has_key ('function'):
                    title_error = CTK.escape_html (command_entry['function'].__name__)
                else:
                    title_error = _("Unknown Error")

                error_content  = CTK.Box ({'class': 'market-commands-exec-error-details'})
                error_content += CTK.RawHTML ("<pre>%s</pre>" %(error_to_HTML(ret['stderr'])))

                details  = CTK.CollapsibleEasy ((_(MORE), _(LESS)))
                details += error_content

                self += CTK.ProgressBar ({'value': percent})
                self += CTK.RawHTML ("<p><b>%s</b>: %s</p>" %(_("Error executing"), title_error))
                self += details
                return

            # Regular
            percent = (command_progress.executed + 1) * 100.0 / (commands_len + 1)
            self += CTK.ProgressBar ({'value': percent})
            self += CTK.Box ({'class': 'market-commands-exec-command'}, CTK.RawHTML ("<p>%s</p>" %(title)))

            # Next step
            if command_progress.executed < commands_len:
                JS = "update = function(){%s}; setTimeout ('update()', %s);"
                self += CTK.RawHTML (js = JS%(command_progress.refresh.JS_to_refresh(), DELAY))
            else:
                self += CTK.RawHTML (js = CTK.DruidContent__JS_to_goto (command_progress.id, command_progress.finished_url))

    def Render (self):
        # Add replacement keys to the log
        for k in self.keys:
            Install_Log.log ("  ${%s} -> '%s'" %(k, self.keys[k]))

        # Start thread
        self.thread.start()

        # Render
        return CTK.Box.Render (self)

    def __init__ (self, commands, finished_url, props={}):
        CTK.Box.__init__ (self, props.copy())
        Replacement_Commons.__init__ (self)

        self.finished_url   = finished_url
        self.commands       = commands
        self.error          = False
        self.executed       = 0
        self.last_popen_ret = None

        self.refresh = CTK.Refreshable ({'id': 'market-commands-exec'})
        self.refresh.register (lambda: CommandProgress.Exec(self).Render())
        self += self.refresh

        self.thread = CommandExec_Thread (self)


class CommandExec_Thread (threading.Thread):
    def __init__ (self, command_progress):
        threading.Thread.__init__ (self)
        self.daemon           = True
        self.command_progress = command_progress

    def run (self):
        for command_entry in self.command_progress.commands:
            if command_entry.has_key ('command'):
                error = self._run_command (command_entry)
            elif command_entry.has_key ('function'):
                error = self._run_function (command_entry)
            else:
                assert False, 'Unknown command type'

            self.command_progress.executed += 1
            if error:
                self.command_progress.error = True
                break

    def _replace (self, txt):
        return self.command_progress.replace (txt)

    def _run_command (self, command_entry):
        command = self._replace (command_entry['command'])
        cd      = command_entry.get('cd')
        env     = command_entry.get('env')
        su      = command_entry.get('su')

        if cd:
            cd = self._replace (cd)

        Install_Log.log ("  %s" %(command))
        if env:
            Install_Log.log ("    (CD)         -> %s" %(cd))
            Install_Log.log ("    (SU)         -> %s" %(su))
            Install_Log.log ("    (CUSTOM ENV) -> %s" %(str(env)))

        ret = popen.popen_sync (command, env=env, cd=cd, su=su)
        self.command_progress.last_popen_ret = ret

        if command_entry.get ('check_ret', True):
            if ret['retcode'] != 0:
                self._report_error (command, env, ret, cd)
                return True

    def _run_function (self, command_entry):
        function = command_entry['function']
        params   = command_entry.get ('params', {}).copy()

        Install_Log.log ("  Function: %s" %(function.__name__))
        if params:
            Install_Log.log ("    (PARAMS) -> %s" %(str(params)))

        try:
            ret = function (**params)
        except:
            ret = {'retcode': 1,
                   'stderr':  traceback.format_exc()}

        self.command_progress.last_popen_ret = ret

        if command_entry.get ('check_ret', True):
            if ret['retcode'] != 0:
                self._report_error (function.__name__, params, ret)
                return True

    def _report_error (self, command, env, ret_exec, cd=None):
        print "="*40
        if env:
            for k in env:
                print "%s='%s' \\"%(k, env[k].replace("'","\\'"))
        print "-"*40
        if cd:
            print cd
        else:
            print os.getcwd()
        print "-"*40
        print command
        print "-"*40
        print ret_exec.get('stdout','')
        print "-"*40
        print ret_exec.get('stderr','')
        print "="*40

