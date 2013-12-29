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
import httplib
import xmlrpclib
import configured

try:
    # Python >= 2.5
    from hashlib import md5
except ImportError:
    # Python <= 2.4
    from md5 import md5

from urllib2 import urlparse
from urllib import splituser, splitpasswd


class CustomTransport(xmlrpclib.Transport):
    proxy_keys = dict (HTTP_PROXY  = 'http',
                       HTTPS_PROXY = 'https')

    def __init__(self, proxies={}, local=False):
        # Warning: Check for parent's init. The xmlrpclib.Transport
        # class does not implement __init__() on Python 2.4
        if hasattr (xmlrpclib.Transport, '__init__'):
            xmlrpclib.Transport.__init__(self)

        self.proxies = dict(((self.proxy_keys[k],v) for k,v in os.environ.items() if k in self.proxy_keys))
        self.proxies.update(proxies)

        self.local      = local
        self.auth       = ''
        self.nc         = 0
        self.user_agent = "Cherokee-admin %s" %(configured.VERSION)

    def make_connection(self, host):
        self.user_pass, self.realhost = splituser(host)
        proto, proxy, p1,p2,p3,p4 = urlparse.urlparse (self.proxies.get('http', ''))
        if proxy and not self.local:
            return httplib.HTTP(proxy)
        else:
            return httplib.HTTP(self.realhost)

    def send_request(self, connection, handler, request_body):
        connection.putrequest ("POST", handler)

    def _parse_response(self, file, sock):
        """Method As in Python 2.5. It is included here because
        the interface changeg, and it was missing in Python 2.7
        and further."""

        p, u = self.getparser()
        while 1:
            if sock:
                response = sock.recv(1024)
            else:
                response = file.read(1024)
            if not response:
                break
            if self.verbose:
                print "body:", repr(response)
            p.feed(response)

        file.close()
        p.close()

        return u.close()

    def _request_internal (self, host, handler, request_body, verbose=0):
        "Re-implementation of xmlrpclib's Transport::request()"

        # own make_conntection()
        h = self.make_connection (host)
        if verbose:
            h.set_debuglevel(1)

        # xmlrpclib's send_request (POST)
        self.send_request (h, handler, request_body)
        self.send_host (h, host)
        self.send_user_agent (h)

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
                h._conn.sock.sendall (request_body)
        except socket.error, (code, msg):
            if not code in (errno.ECONNRESET, errno.ECONNABORTED, errno.EPIPE):
                raise

        # The rest is as in the original Transport::request() method
        errcode, errmsg, headers = h.getreply()
        if errcode != 200:
            raise xmlrpclib.ProtocolError(
                host + handler,
                errcode, errmsg,
                headers
                )

        self.verbose = verbose
        try:
            sock = h._conn.sock
        except AttributeError:
            sock = None

        return self._parse_response (h.getfile(), sock)

    def request (self, host, handler, request_body, verbose=0):
        for count in range(3):
            try:
                return self._request_internal (host, handler, request_body, verbose)
            except xmlrpclib.ProtocolError, ex:
                if ex.errcode == 401:
                    auth, params = ex.headers['WWW-Authenticate'].split(' ', 1)
                    self.uri    = handler
                    self.auth   = auth.lower()
                    self.params = dict([[n.strip() for n in i.split('=',1)] for i in params.split(',')])
        raise ex

    def send_host(self, connection, host):
        connection.putheader('Host', self.realhost)
        if self.auth=='basic':
            if self.user_pass:
                user_pass = base64.encodestring(self.user_pass).strip()
                connection.putheader('Authorization', 'Basic %s' % user_pass)

        elif self.auth=='digest':
            if self.user_pass:
                self.nc += 1
                username, password = self.user_pass.split(':')

                uri   = self.uri
                realm = self.params['realm'].strip('"')
                nonce = self.params['nonce'].strip('"')
                qop   = self.params['qop'].strip('"')

                a1 = md5('%(username)s:%(realm)s:%(password)s' % locals()).hexdigest()
                a2 = md5('POST:%(uri)s' % locals()).hexdigest()

                nc = '%08d' % self.nc
                chars = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'
                cnonce = ''.join([random.choice(chars) for i in range(16)])
                response = md5('%(a1)s:%(nonce)s:%(nc)s:%(cnonce)s:%(qop)s:%(a2)s' % locals()).hexdigest()

                params = []
                params.append('username=%s' % username)
                params.append('realm="%s"' % realm)
                params.append('nonce="%s"' % nonce)
                params.append('uri=%s' % uri)
                params.append('algorithm=%s' % self.params['algorithm'])
                params.append('qop=%s' % qop)
                params.append('nc=%s' % nc)
                params.append('cnonce="%s"' % cnonce)
                params.append('response="%s"' % response)

                connection.putheader('Authorization', 'Digest %s' % ', '.join(params))


class XmlRpcServer (xmlrpclib.ServerProxy):
    def __init__(self, url, user='', password='', local=False):
        protocol,domain,path, d1,params,d3 = urlparse.urlparse(url)
        if params:
            params = '?'+params

        user_pass = ''
        if user:
            user_pass = ':'.join([user,password]) + '@'

        href = '%(protocol)s://%(user_pass)s%(domain)s%(path)s%(params)s' % locals()
        xmlrpclib.ServerProxy.__init__ (self, href, transport=CustomTransport(local=local), allow_none=True)


#if __name__=='__main__':
#    server = XmlRpcServer('http://localhost:8080/proud', 'admin', 'secretPassword')
#    print server.add(2,3)
