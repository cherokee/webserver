"""
pyscgi.py - Portable SCGI implementation

This module has been written as part of the Cherokee project:
               http://www.cherokee-project.com/
"""

# Copyright (c) 2006, Alvaro Lopez Ortega <alvaro@alobbs.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in
#   the documentation and/or other materials provided with the
#   distribution.
# * The name "Alvaro Lopez Ortega" may not be used to endorse or
#   promote products derived from this software without specific prior
#   written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import SocketServer

__version__ = '1.0'
__author__  = 'Alvaro Lopez Ortega'


class SCGIHandler (SocketServer.BaseRequestHandler):
    def __init__ (self, request, client_address, server):
        self.env  = {}
        self.post = None
        SocketServer.BaseRequestHandler.__init__ (self, request, client_address, server)

    def __read_netstring_size (self):
        size = ""
        while 1:
            c = self.input.read(1)
            if c == ':':
                break
            elif not c:
                raise IOError, 'Malformed netstring'
            size += c
        return long(size)

    def __read_netstring (self):
        data = ""
        size = self.__read_netstring_size()
        while size > 0:
            s = self.input.read(size)
            if not s:
                raise IOError, 'Malformed netstring'
            data += s
            size -= len(s)
            if self.input.read(1) != ',':
                raise IOError, 'Missing netstring terminator'
        return data

    def __read_env (self):
        headers = self.__read_netstring()
        items   = headers.split('\0')[:-1]
        itemsn  = len(items)
        if itemsn % 2 != 0:
            raise Exception, 'Malformed headers'
        for i in range(0, itemsn, 2):
            self.env[items[i]] = items[i+1]

    def handle_post (self):
        if not self.env.has_key('CONTENT_LENGTH'):
            return
        length = int(self.env['CONTENT_LENGTH'])
        self.post = self.input.read(length)

    def handle (self):
        self.input = self.request.makefile('r')
        self.output = self.request.makefile('w')

        self.__read_env()
        self.handle_request()
        self.output.close()
        self.input.close()

    def handle_request (self):
        self.output.write("Content-Type: text/plain\r\n\r\n")
        self.output.write("handle_request() should be overridden")


class SCGIServer(SocketServer.ThreadingTCPServer):
    def __init__(self, handler_class=SCGIHandler, host="", port=4000):
        self.allow_reuse_address = True
        SocketServer.ThreadingTCPServer.__init__ (self, (host, port), handler_class)

class SCGIServerFork (SocketServer.ForkingTCPServer):
    def __init__(self, handler_class=SCGIHandler, host="", port=4000):
        self.allow_reuse_address = True
        SocketServer.ForkingTCPServer.__init__ (self, (host, port), handler_class)


def ServerFactory (threading=False, *args, **kargs):
    if threading:
        return SCGIServer(*args, **kargs)
    else:
        return SCGIServerFork(*args, **kargs)
