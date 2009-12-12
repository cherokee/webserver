#!/usr/bin/env python

import os
import re
import sys
import stat
import time
import pyscgi
import thread
import signal
import socket
import gettext
import traceback

# Application modules
#
from config import *
from configured import *
from Post import *
from PageStatus import *
from PageGeneral import *
from PageIcon import *
from PageMime import *
from PageVServer import *
from PageVServers import *
from PageEntry import *
from PageAdvanced import *
from PageFeedback import *
from PageError import *
from PageNewConfig import *
from PageAjaxUpdate import *
from PageInfoSource import *
from CherokeeManagement import *
from config_version import *

# Constants
#
MODIFIED_CHECK_ELAPSE = 1

# Globals
#
cfg = None
SELECTED_LANGUAGE = False

# Request handler
#
class Handler(pyscgi.SCGIHandler):
    def __init__ (self, *args, **kwords):
        pyscgi.SCGIHandler.__init__ (self, *args)

    def handle_request (self):
        global cfg

        page    = None
        headers = ""
        body    = ""
        status  = "200 OK"
        uri     = self.env['REQUEST_URI']

        # Translation
        if (not SELECTED_LANGUAGE and
            self.env.has_key('HTTP_ACCEPT_LANGUAGE')):
            try:
                langs = self.env['HTTP_ACCEPT_LANGUAGE']
            except:
                langs = None

            select_language (langs)

        # Ensure that the configuration file is writable
        if not cfg.has_tree():
            if not uri.startswith('/create_config'):
                page = PageNewConfig (cfg)
        elif not cfg.is_writable():
            page = PageError (cfg, PageError.CONFIG_NOT_WRITABLE)
        elif not os.path.isdir(CHEROKEE_ICONSDIR):
            page = PageError (cfg, PageError.ICONS_DIR_MISSING)

        if page:
            body = page.HandleRequest (uri, Post())

            if body and body[0] == '/':
                self.send ("Status: 302 Moved Temporarily\r\n" + \
                           "Location: %s\r\n\r\n" % (body))
                return
            else:
                self.send ('Status: 200 OK\r\n\r\n' + body)
                return

        # Check the URL
        if uri.startswith('/general'):
            page = PageGeneral(cfg)
        elif uri.startswith('/icon'):
            page = PageIcon(cfg)
        elif uri.startswith('/mime'):
            page = PageMime(cfg)
        elif uri.startswith('/advanced'):
            page = PageAdvanced(cfg)
        elif uri.startswith('/feedback'):
            page = PageFeedback(cfg)
        elif uri == '/vserver' or \
             uri == '/vserver/' or \
             uri == '/vserver/ajax_update' or\
             uri.startswith('/vserver/wizard'):
            page = PageVServers(cfg)
        elif uri.startswith('/vserver/'):
            if "/rule/" in uri:
                page = PageEntry(cfg)
            else:
                page = PageVServer(cfg)
        elif uri.startswith('/source'):
            page = PageInfoSource(cfg)
        elif uri.startswith('/apply_ajax'):
            self.handle_post()
            post = Post(self.post)
            post_restart = post.get_val('restart')

            manager = cherokee_management_get (cfg)
            manager.save (restart = post_restart)
            cherokee_management_reset()

            body = _('Configuration saved.')
            if post_restart == 'graceful':
                body += _(' Graceful restart performed.')
            elif post_restart == 'hard':
                body += _(' Hard restart performed.')

        elif uri.startswith('/change_language'):
            self.handle_post()
            post = Post(self.post)
            post_lang = post.get_val('language')

            select_language(post_lang)
            body = '/'

        elif uri.startswith('/launch'):
            manager = cherokee_management_get (cfg)
            error = manager.launch()
            if error:
                page = PageError_LaunchFail (cfg, error)
            else:
                body = "/"
        elif uri.startswith('/stop'):
            manager = cherokee_management_get (cfg)
            manager.stop()
            cherokee_management_reset()
            body = "/"
        elif uri.startswith('/create_config'):
            tmp = PageNewConfig (cfg)
            body = tmp.HandleRequest(uri, Post(''))
            if body:
                cfg = Config (cfg.file)
            else:
                tmp = PageError (cfg, PageError.CONFIG_NOT_WRITABLE)
                body = tmp._op_render()

        elif uri.startswith('/ajax/update'):
            page = PageAjaxUpdate (cfg)
        elif uri.startswith('/rule'):
            page = RuleOp(cfg)
        elif uri == '/':
            page = PageStatus(cfg)
        else:
            body = "/"

        # Handle post
        self.handle_post()
        post = Post (self.post)

        # Execute page
        if page:
            try:
                body = page.HandleRequest(uri, post)
            except:
                trace = traceback.format_exc()
                page = PageInternelError (trace)
                body = page.HandleRequest (uri, Post())
                self.send ('Status: 500 Internal Server Error\r\n\r\n' + body)
                return

        # Is it a redirection?
        if body[0] == '/':
            status   = "302 Moved Temporarily"
            headers += "Location: %s\r\n" % (body)

        # Send result
        content = 'Status: %s\r\n' % (status) + \
                  headers + '\r\n' + body
        return self.send (content)


def select_language (langs):
    global SELECTED_LANGUAGE

    if langs:
        languages = [l for s in langs.split(',') for l in s.split(';') if not '=' in l]
        try:
            gettext.translation('cherokee', LOCALEDIR, languages).install()
        except:
            pass

    SELECTED_LANGUAGE = True


# Server
#
def main():
    # Gettext initialization
    gettext.install('cherokee')

    # Read the arguments
    try:
        scgi_port = sys.argv[1]
        cfg_file  = sys.argv[2]
    except:
        print _("Incorrect parameters: PORT CONFIG_FILE")
        raise SystemExit

    # Try to avoid zombie processes
    if hasattr(signal, "SIGCHLD"):
        signal.signal(signal.SIGCHLD, signal.SIG_IGN)

    # Move to the server directory
    pathname, scriptname = os.path.split(sys.argv[0])
    os.chdir(os.path.abspath(pathname))

    # SCGI server
    if scgi_port.isdigit():
        srv = pyscgi.ServerFactory (True, handler_class=Handler, host="127.0.0.1", port=int(scgi_port))
    else:
        # Remove the unix socket if it already exists
        try:
            mode = os.stat (scgi_port)[stat.ST_MODE]
            if stat.S_ISSOCK(mode):
                print "Removing an old '%s' unix socket.." %(scgi_port)
                os.unlink (scgi_port)
        except OSError:
            pass

        srv = pyscgi.ServerFactory (True, handler_class=Handler, unix_socket=scgi_port)

    srv.socket.settimeout (MODIFIED_CHECK_ELAPSE)

    # Read configuration file
    global cfg
    cfg = Config(cfg_file)

    # Update the configuration file if needed
    config_version_update_cfg (cfg)

    # Let the user know what is going on
    version = VERSION
    pid     = os.getpid()

    if scgi_port.isdigit():
        print _("Server %(version)s running.. PID=%(pid)d Port=%(scgi_port)s") % (locals())
    else:
        print _("Server %(version)s running.. PID=%(pid)d Socket=%(scgi_port)s") % (locals())

    # Iterate until the user exists
    try:
        while True:
            srv.handle_request()
    except KeyboardInterrupt:
        print "\r", _("Server exiting..")

    srv.server_close()

if __name__ == '__main__':
    main()
