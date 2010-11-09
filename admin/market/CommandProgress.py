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
import CTK
import popen
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


def replacement_cmd (command):
    # Install user (root, whell, admin, etc)
    root_user  = InstallUtil.get_installation_UID()
    root_group = InstallUtil.get_installation_GID()

    # Server user (www-data, server, nobody, etc)
    server_user  = CTK.cfg.get_val ('server!user',  str(os.getuid()))
    server_group = CTK.cfg.get_val ('server!group', str(os.getgid()))

    # Directories
    app_root = CTK.cfg.get_val ('tmp!market!install!root')

    # Replacements
    command = command.replace('${web_user}',   server_user)
    command = command.replace('${web_group}',  server_group)
    command = command.replace('${root_user}',  root_user)
    command = command.replace('${root_group}', root_group)
    command = command.replace('${app_root}',   app_root)

    return command


class CommandProgress (CTK.Box):
    class Exec (CTK.Container):
        def __init__ (self, command_progress):
            CTK.Container.__init__ (self)

            commands_len = len(command_progress.commands)
            if command_progress.error or \
               command_progress.executed >= commands_len:
                command_entry = command_progress.commands [command_progress.executed - 1]
            else:
                command_entry = command_progress.commands [command_progress.executed]
            command = replacement_cmd (command_entry['command'])

            # Error
            if command_progress.error:
                ret     = command_progress.last_popen_ret
                percent = command_progress.executed * 100.0 / (commands_len + 1)

                error_content  = CTK.Box ({'class': 'market-commands-exec-error-details'})
                error_content += CTK.RawHTML ("<pre>%s</pre>" %(error_to_HTML(ret['stderr'])))

                details  = CTK.CollapsibleEasy ((_(MORE), _(LESS)))
                details += error_content

                self += CTK.ProgressBar ({'value': percent})
                self += CTK.RawHTML ("<p><b>%s</b>: %s</p>" %(_("Error executing"), command))
                self += details
                return

            # Regular
            percent = (command_progress.executed + 1) * 100.0 / (commands_len + 1)
            self += CTK.ProgressBar ({'value': percent})
            self += CTK.Box ({'class': 'market-commands-exec-command'}, CTK.RawHTML ("<p>%s</p>" %(command_entry['command'])))

            # Next step
            if command_progress.executed < commands_len:
                JS = "update = function(){%s}; setTimeout ('update()', %s);"
                self += CTK.RawHTML (js = JS%(command_progress.refresh.JS_to_refresh(), DELAY))
            else:
                self += CTK.RawHTML (js = CTK.DruidContent__JS_to_goto (command_progress.id, command_progress.finished_url))


    def __init__ (self, commands, finished_url):
        CTK.Box.__init__ (self)

        self.finished_url    = finished_url
        self.commands        = commands
        self.error           = False
        self.executed        = 0
        self.last_popen_ret  = None

        self.refresh = CTK.Refreshable ({'id': 'market-commands-exec'})
        self.refresh.register (lambda: CommandProgress.Exec(self).Render())
        self += self.refresh

        self.thread = CommandExec_Thread (self)
        self.thread.start()


class CommandExec_Thread (threading.Thread):
    def __init__ (self, command_progress):
        threading.Thread.__init__ (self)
        self.command_progress = command_progress

    def run (self):
        for command_entry in self.command_progress.commands:
            error = self._run_command (command_entry)
            self.command_progress.executed += 1
            if error:
                self.command_progress.error = True
                break

    def _run_command (self, command_entry):
        command = replacement_cmd (command_entry['command'])
        env     = command_entry.get('env')

        Install_Log.log ("  %s" %(command))
        if env:
            Install_Log.log ("    (CUSTOM ENV) -> %s" %(str(env)))

        ret = popen.popen_sync (command, env=env)
        self.command_progress.last_popen_ret = ret

        if command_entry.get ('check_ret', True):
            if ret['retcode'] != 0:
                self._report_error (command, env, ret)
                return True

    def _report_error (self, command, env, ret_exec):
        print "="*40
        if env:
            for k in env:
                print "%s=%s \\"%(k, env[k])
        print command
        print "-"*40
        print ret_exec['stdout']
        print "-"*40
        print ret_exec['stderr']
        print "="*40

