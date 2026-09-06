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
import errno
import base64
import socket
import random
import http.client
import xmlrpc.client
import configured
from typing import Any

from hashlib import md5

from urllib.parse import unquote, urlsplit


class CustomTransport(xmlrpc.client.Transport):
    proxy_keys = dict (HTTP_PROXY  = 'http',
                       HTTPS_PROXY = 'https')

    def __init__(self, proxies={}, local=False):
        # Warning: Check for parent's init. The xmlrpclib.Transport
        # Keep the transport compatible with the supported Python 3 runtime.
        if hasattr (xmlrpc.client.Transport, '__init__'):
            xmlrpc.client.Transport.__init__(self)

        self.proxies = dict(((self.proxy_keys[k],v) for k,v in list(os.environ.items()) if k in self.proxy_keys))
        self.proxies.update(proxies)

        self.local      = local
        self.auth       = ''
        self.nc         = 0
        self.user_agent = f"Cherokee-admin {configured.VERSION}"

    def make_connection(self, host):
        parsed_host = urlsplit(f'//{host}')
        if parsed_host.username is not None:
            username = unquote(parsed_host.username)
            password = unquote(parsed_host.password or '')
            self.user_pass = f'{username}:{password}'
        else:
            self.user_pass = ''
        self.realhost = parsed_host.netloc.rsplit('@', 1)[-1]

        parsed_proxy = urlsplit(self.proxies.get('http', ''))
        proxy = parsed_proxy.netloc
        return http.client.HTTPConnection(proxy or self.realhost)

    def _send_post_request(self, connection: Any, handler: str,
                           request_body: bytes) -> None:
        connection.putrequest ("POST", handler)

    def _parse_response(self, file, sock=None):
        """Compatibility method for the current Python 3 HTTP interface.
        and further."""

        p, u = self.getparser()
        while 1:
            response = file.read(1024)
            if not response:
                break
            if self.verbose:
                print("body:", repr(response))
            p.feed(response)

        file.close()
        p.close()

        return u.close()

    def _request_internal (self, host, handler, request_body, verbose=0):
        "Re-implementation of xmlrpclib's Transport::request()"

        # own make_conntection()
        h = self.make_connection (host)
        h.set_debuglevel(int(bool(verbose)))

        # xmlrpclib's send_request (POST)
        self._send_post_request (h, handler, request_body)
        self.send_host (h, host)
        h.putheader('User-Agent', self.user_agent)

        # CUSTOM: xmlrpclib's send_content -- The original version
        # closes the connection if a EPIPE error is produced without
        # tryig to read the response (if any). The server might have
        # replied an error without waiting the POST to be fully sent.
        #
        h.putheader ("Content-Type", "text/xml")
        h.putheader ("Content-Length", str(len(request_body)))
        h.endheaders()

        try:
            if request_body:
                h.send(request_body)
        except OSError as exc:
            if exc.errno not in (errno.ECONNRESET, errno.ECONNABORTED, errno.EPIPE):
                raise

        # The rest is as in the original Transport::request() method
        response = h.getresponse()
        if response.status != 200:
            raise xmlrpc.client.ProtocolError(
                host + handler,
                response.status, response.reason,
                response.headers
                )

        self.verbose = verbose
        return self._parse_response(response)

    def request (self, host, handler, request_body, verbose=0):
        last_error = RuntimeError('XML-RPC request failed')
        for count in range(3):
            try:
                return self._request_internal (host, handler, request_body, verbose)
            except xmlrpc.client.ProtocolError as ex:
                last_error = ex
                if ex.errcode == 401:
                    challenge = ex.headers.get('WWW-Authenticate', '')
                    auth, params = challenge.split(' ', 1)
                    self.uri    = handler
                    self.auth   = auth.lower()
                    self.params = dict([[n.strip() for n in i.split('=',1)] for i in params.split(',')])
        raise last_error

    def send_user_agent(self, connection):
        connection.putheader('User-Agent', self.user_agent)

    def send_host(self, connection, host):
        connection.putheader('Host', self.realhost)
        if self.auth=='basic':
            if self.user_pass:
                user_pass = base64.b64encode(self.user_pass.encode()).decode().strip()
                connection.putheader('Authorization', f'Basic {user_pass}')

        elif self.auth=='digest':
            if self.user_pass:
                self.nc += 1
                username, password = self.user_pass.split(':')

                uri   = self.uri
                realm = self.params['realm'].strip('"')
                nonce = self.params['nonce'].strip('"')
                qop   = self.params['qop'].strip('"')

                a1 = md5(f"{locals()['username']}:{locals()['realm']}:{locals()['password']}".encode()).hexdigest()
                a2 = md5(f"POST:{locals()['uri']}".encode()).hexdigest()

                nc = f'{int(self.nc):08}'
                chars = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'
                cnonce = ''.join([random.choice(chars) for i in range(16)])
                response = md5(f"{locals()['a1']}:{locals()['nonce']}:{locals()['nc']}:{locals()['cnonce']}:{locals()['qop']}:{locals()['a2']}".encode()).hexdigest()

                params = []
                params.append(f'username={username}')
                params.append(f'realm="{realm}"')
                params.append(f'nonce="{nonce}"')
                params.append(f'uri={uri}')
                params.append(f"algorithm={self.params['algorithm']}")
                params.append(f'qop={qop}')
                params.append(f'nc={nc}')
                params.append(f'cnonce="{cnonce}"')
                params.append(f'response="{response}"')

                connection.putheader('Authorization', f"Digest {', '.join(params)}")


class XmlRpcServer (xmlrpc.client.ServerProxy):
    def __init__(self, url, user='', password='', local=False):
        parsed_url = urlsplit(url)
        protocol, domain, path = parsed_url.scheme, parsed_url.netloc, parsed_url.path
        params = parsed_url.query
        if params:
            params = '?'+params

        user_pass = ''
        if user:
            user_pass = ':'.join([user,password]) + '@'

        href = f"{locals()['protocol']}://{locals()['user_pass']}{locals()['domain']}{locals()['path']}{locals()['params']}"
        xmlrpc.client.ServerProxy.__init__ (self, href, transport=CustomTransport(local=local), allow_none=True)


#if __name__=='__main__':
#    server = XmlRpcServer('http://localhost:8080/proud', 'admin', 'secretPassword')
#    print server.add(2,3)
