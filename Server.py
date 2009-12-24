# CTK: Cherokee Toolkit
#
# Authors:
#      Alvaro Lopez Ortega <alvaro@alobbs.com>
#
# Copyright (C) 2009 Alvaro Lopez Ortega
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
import json
import pyscgi

from Post import Post


class HTTP_Response:
    DESC = {
        100: 'CONTINUE',
        101: 'SWITCHING PROTOCOLS',
        200: 'OK',
        201: 'CREATED',
        202: 'ACCEPTED',
        203: 'NON-AUTHORITATIVE INFORMATION',
        204: 'NO CONTENT',
        205: 'RESET CONTENT',
        206: 'PARTIAL CONTENT',
        300: 'MULTIPLE CHOICES',
        301: 'MOVED PERMANENTLY',
        302: 'FOUND',
        303: 'SEE OTHER',
        304: 'NOT MODIFIED',
        305: 'USE PROXY',
        306: 'RESERVED',
        307: 'TEMPORARY REDIRECT',
        400: 'BAD REQUEST',
        401: 'UNAUTHORIZED',
        402: 'PAYMENT REQUIRED',
        403: 'FORBIDDEN',
        404: 'NOT FOUND',
        405: 'METHOD NOT ALLOWED',
        406: 'NOT ACCEPTABLE',
        407: 'PROXY AUTHENTICATION REQUIRED',
        408: 'REQUEST TIMEOUT',
        409: 'CONFLICT',
        410: 'GONE',
        411: 'LENGTH REQUIRED',
        412: 'PRECONDITION FAILED',
        413: 'REQUEST ENTITY TOO LARGE',
        414: 'REQUEST-URI TOO LONG',
        415: 'UNSUPPORTED MEDIA TYPE',
        416: 'REQUESTED RANGE NOT SATISFIABLE',
        417: 'EXPECTATION FAILED',
        500: 'INTERNAL SERVER ERROR',
        501: 'NOT IMPLEMENTED',
        502: 'BAD GATEWAY',
        503: 'SERVICE UNAVAILABLE',
        504: 'GATEWAY TIMEOUT',
        505: 'HTTP VERSION NOT SUPPORTED',
        }

    def __init__ (self, error=200, headers=[], body=''):
        self.error   = error
        self.headers = headers
        self.body    = body

    def __setitem__ (self, key, value):
        self.headers.append ("%s: %s"%(key, str(value)))

    def __str__ (self):
        # Add content length
        if not "Content-Length:" in ''.join(self.headers).lower():
            self['Content-Length'] = len(self.body)

        # Build the HTTP response
        hdr  = "%d %s\r\n" %(self.error, HTTP_Response.DESC[self.error])
        hdr += "Status: %d\r\n" %(self.error)
        hdr += "\r\n".join (self.headers) + '\r\n'

        # No body replies: RFC2616 4.3
        if self.error in (100, 101, 204, 304):
            return hdr

        return hdr + '\r\n' + self.body


class HTTP_Error (HTTP_Response):
    def __init__ (self, error=500):
        HTTP_Response.__init__ (self, error)

        self.body  = '<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">\r\n'
        self.body += "<html><head><title>Error %d: %s</title></head>\n" %(error, HTTP_Response.DESC[error])
        self.body += '<body><h1>Error %d: %s</h1></body>\n' %(error, HTTP_Response.DESC[error])
        self.body += "</html>"


class PostValidator:
    def __init__ (self, post, validation_list):
        self.post            = post
        self.validation_list = validation_list

    def Validate (self):
        errors  = {}
        updates = {}

        for key in self.post:
            key_done = False

            for regex, func in self.validation_list:
                if key_done: break

                if re.match(regex, key):
                    key_done = True

                    for n in range(len(self.post[key])):
                        val = self.post[key][n]
                        try:
                            tmp = func (val)
                        except Exception, e:
                            errors[key] = str(e)
                            break

                        if tmp:
                            self.post[key][n] = tmp
                            updates[key] = tmp

        if errors or updates:
            return {'ret': "unsatisfactory",
                    'errors':  errors,
                    'updates': updates}


class ServerHandler (pyscgi.SCGIHandler):
    def __init__ (self, *args):
        pyscgi.SCGIHandler.__init__ (self, *args)

    def _process_post (self):
        pyscgi.SCGIHandler.handle_post(self)
        post = Post (self.post)
        return post

    def _do_handle (self):
        # Read the URL
        url = self.env['REQUEST_URI']

        # Get a copy of the server (it did fork!)
        server = get_server()

        for published in server._web_paths:
            if re.match (published._regex, url):
                # POST
                if published._method == 'POST':
                    post = self._process_post()
                    published._kwargs['post'] = post

                    # Validate
                    validator = PostValidator (post, published._validation)
                    errors = validator.Validate()
                    if errors:
                        resp = HTTP_Response(200, body=json.dumps(errors))
                        resp['Content-Type'] = "application/json"
                        return resp

                # Execute handler
                ret = published (**published._kwargs)

                # Deal with the returned info
                if type(ret) == str:
                    return HTTP_Response(200, body=ret)

                elif type(ret) == dict:
                    info = json.dumps(ret)
                    resp = HTTP_Response(200, body=info)
                    resp['Content-Type'] = "application/json"
                    return resp

                else:
                    return HTTP_Response(200, body=ret)

        # Not found
        return HTTP_Error (404)

    def handle_request (self):
        content = self._do_handle()
        self.send(str(content))


class Server:
    def __init__ (self):
        self._web_paths = []
        self._scgi      = None
        self._is_init   = False

    def init_server (self, *args, **kwargs):
        # Is it already init?
        if self._is_init:
            return
        self._is_init = True

        # Instance SCGI server
        self._scgi = pyscgi.ServerFactory (*args, **kwargs)

    def sort_routes (self):
        def __cmp(x,y):
            lx = len(x._regex)
            ly = len(y._regex)
            return cmp(ly,lx)

        self._web_paths.sort(__cmp)

    def add_route (self, route_obj):
        self._web_paths.append (route_obj)
        self.sort_routes()

    def serve_forever (self):
        try:
            while True:
                # Handle request
                self._scgi.handle_request()
        except KeyboardInterrupt:
            print "\r", _("Server exiting..")
            self._scgi.server_close()


#
# Helpers
#

__global_server = None
def get_server():
    global __global_server
    if not __global_server:
        __global_server = Server ()

    return __global_server

def run (*args, **kwargs):
    srv = get_server()

    kwargs['handler_class'] = ServerHandler
    srv.init_server (*args, **kwargs)
    srv.serve_forever()


class Publish_FakeClass:
    def __init__ (self, func):
        self.__func = func

    def __call__ (self, *args, **kwargs):
        return self.__func (*args, **kwargs)


def publish (regex_url, klass, **kwargs):
    # Instance object
    if type(klass) == type(lambda: None):
        obj = Publish_FakeClass (klass)
    else:
        obj = klass()

    # Set internal properties
    obj._kwargs     = kwargs
    obj._regex      = regex_url
    obj._validation = kwargs.pop('validation', [])
    obj._method     = kwargs.pop('method', None)

    # Register
    server = get_server()
    server.add_route (obj)


