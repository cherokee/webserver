"""
pyscgi.py - Portable SCGI implementation

This module has been written as part of the Cherokee project:
               http://www.cherokee-project.com/
"""

# Copyright (C) 2006-2014 Alvaro Lopez Ortega
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
import traceback
import socket
import errno
import time
import sys
import os

__version__   = '1.16.1'
__author__    = 'Alvaro Lopez Ortega'
__copyright__ = 'Copyright 2011, Alvaro Lopez Ortega'
__license__   = 'BSD'


class SCGIHandler (SocketServer.StreamRequestHandler):
    def __init__ (self, request, client_address, server):
        self.env    = {}
        self.post   = None
        SocketServer.StreamRequestHandler.__init__ (self, request, client_address, server)

    def __safe_read (self, length):
        info = ''
        while True:
            if len(info) >= length:
                return info

            chunk = None
            try:
                to_read = length - len(info)
                chunk = os.read (self.rfile.fileno(), to_read)
                if not len(chunk):
                    return info
                info += chunk
            except OSError, e:
                if e.errno in (errno.EAGAIN, errno.EWOULDBLOCK, errno.EINPROGRESS):
                    if chunk:
                        info += chunk
                        continue
                    time.sleep(0.001)
                    continue
                raise

    def send(self, buf):
        pending = len(buf)
        offset  = 0

        while True:
            if not pending:
                return

            try:
                sent = os.write (self.wfile.fileno(), buf[offset:])
                pending -= sent
                offset  += sent
            except OSError, e:
                if e.errno in (errno.EAGAIN, errno.EWOULDBLOCK, errno.EINPROGRESS):
                    time.sleep(0.001)
                    continue
                raise

    def __read_netstring_size (self):
        size = ""
        while True:
            c = self.__safe_read(1)
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
            s = self.__safe_read(size)
            if not s:
                raise IOError, 'Malformed netstring'
            data += s
            size -= len(s)
            if self.__safe_read(1) != ',':
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
        if self.post:
            return

        if not self.env.has_key('CONTENT_LENGTH'):
            return

        length = int(self.env['CONTENT_LENGTH'])
        self.post = self.__safe_read(length)

    def handle (self):
        self.__read_env()

        try:
            self.handle_request()
        except:
            if sys.exc_type != SystemExit:
                traceback.print_exc()  # Print the error

        try: # Closes wfile and rfile
            self.finish()
        except: pass

        try: # Send a FIN signal
            self.request.shutdown (socket.SHUT_WR)
        except: pass

        try: # Wait for the client's ACK+FIN
            while True:
                if not self.request.recv(1):
                    break;
        except: pass

        try: # Send either ACK, or RST
            self.request.close()
        except: pass


    def handle_request (self):
        self.send('Status: 200 OK\r\n')
        self.send("Content-Type: text/plain\r\n\r\n")
        self.send("handle_request() should be overridden")


class ThreadingMixIn_Custom (SocketServer.ThreadingMixIn):
    def set_synchronous (self, sync):
        assert type(sync) == bool
        self.syncronous = sync

    def process_request (self, request, client_address):
        if hasattr(self, 'syncronous') and self.syncronous:
            return self.process_request_thread (request, client_address)

        return SocketServer.ThreadingMixIn.process_request (self, request, client_address)


class ThreadingUnixStreamServer_Custom (ThreadingMixIn_Custom, SocketServer.UnixStreamServer):
    pass

class ThreadingTCPServer_Custom (ThreadingMixIn_Custom, SocketServer.TCPServer):
    def server_bind (self):
        host, port = self.server_address

        # Binding to a IP/host
        if host:
            return self.server_bind_multifamily()

        # Binding all interfaces
        return SocketServer.TCPServer.server_bind (self)

    def server_bind_multifamily (self):
        # Loop over the different addresses of 'host'
        host, port = self.server_address
        addresses = socket.getaddrinfo (host, port, socket.AF_UNSPEC,
                                        socket.SOCK_STREAM, 0, socket.AI_PASSIVE)

        # Find a suitable address
        s = None
        for res in addresses:
            af, socktype, protocol, canonicalname, sa = res

            # Create socket
            try:
                s = socket.socket (af, socktype, protocol)
            except socket.error:
                s = None
                continue

            # Bind
            try:
                if self.allow_reuse_address:
                    s.setsockopt (socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind(sa)
            except socket.error:
                s.close()
                s = None
                continue

            break

        # If none successfully bind report error
        if not s:
            raise socket.error, "Could not create server socket"

        self.socket = s

        # Finally, fix the server_address
        self.server_address = self.socket.getsockname()


# TCP port
#
class SCGIServer (ThreadingTCPServer_Custom):
    def __init__(self, handler_class=SCGIHandler, host="", port=4000):
        self.allow_reuse_address = True
        ThreadingTCPServer_Custom.__init__ (self, (host, port), handler_class)

class SCGIServerFork (SocketServer.ForkingTCPServer):
    def __init__(self, handler_class=SCGIHandler, host="", port=4000):
        self.allow_reuse_address = True
        SocketServer.ForkingTCPServer.__init__ (self, (host, port), handler_class)

# Unix socket
#
class SCGIUnixServer (ThreadingUnixStreamServer_Custom):
    def __init__(self, unix_socket, handler_class=SCGIHandler):
        self.allow_reuse_address = True
        ThreadingUnixStreamServer_Custom.__init__ (self, unix_socket, handler_class)

class SCGIUnixServerFork (SocketServer.UnixStreamServer):
    def __init__(self, unix_socket, handler_class=SCGIHandler):
        self.allow_reuse_address = True
        SocketServer.UnixStreamServer.__init__ (self, unix_socket, handler_class)


def ServerFactory (threading=False, *args, **kargs):
    unix_socket = kargs.get('unix_socket', None)

    if threading:
        if unix_socket:
            return SCGIUnixServer (*args, **kargs)
        else:
            return SCGIServer(*args, **kargs)
    else:
        if unix_socket:
            return SCGIUnixServerFork(*args, **kargs)
        else:
            return SCGIServerFork(*args, **kargs)
