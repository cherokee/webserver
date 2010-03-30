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

    def __init__ (self, error=200, headers=None, body=''):
        self.error   = error
        self.body    = body
        self.headers = ([],headers)[bool(headers)]

    def __add__ (self, text):
        assert type(text) == str
        self.body += text
        return self

    def __setitem__ (self, key, value):
        self.headers.append ("%s: %s"%(key, str(value)))

    def __str__ (self):
        all_lower = ''.join(self.headers).lower()

        # Add default headers
        if not "content-length:" in all_lower:
            self['Content-Length'] = len(self.body)

        if not "content-type:" in all_lower:
            self['Content-Type'] = 'text/html'

        # Build the HTTP response
        hdr  = "Status: %d\r\n" %(self.error)
        hdr += "X-Powered-By: Cherokee and CTK\r\n"
        hdr += "\r\n".join (self.headers) + '\r\n'

        # No body replies: RFC2616 4.3
        if self.error in (100, 101, 204, 304):
            return hdr

        return hdr + '\r\n' + self.body


class HTTP_Error (HTTP_Response):
    def __init__ (self, error=500, desc=None):
        HTTP_Response.__init__ (self, error)
        self.body  = '<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">\r\n'
        self.body += "<html><head><title>Error %d: %s</title></head>\n" %(error, HTTP_Response.DESC[error])
        self.body += '<body><h1>Error %d: %s</h1>\n' %(error, HTTP_Response.DESC[error])
        if desc:
            self.body += "<p>%s</p>"%(desc)
        self.body += "</body></html>"


class HTTP_Redir (HTTP_Response):
    def __init__ (self, location, error=307):
        HTTP_Response.__init__ (self, error)
        self['Location'] = location
        self.body += 'Redirecting to <a href="%s">%s</a>.' %(location, location)


class HTTP_XSendfile (HTTP_Response):
    def __init__ (self, location, error=200):
        HTTP_Response.__init__ (self, error)
        self['X-Sendfile'] = location
