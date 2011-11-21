from base import *

NICK    = "test-2920"
MAX_AGE = 123456

CONF = """
vserver!2920!nick = %(NICK)s
vserver!2920!document_root = %(droot)s
vserver!2920!hsts = 1
vserver!2920!hsts!max_age = %(MAX_AGE)s
vserver!2920!rule!1!match = default
vserver!2920!rule!1!handler = dirlist
"""

# 6.2.  HTTP Request Type
#
#   If a HSTS Host receives a HTTP request message over a non-secure
#   transport, it SHOULD send a HTTP response message containing a
#   Status-Code of 301 and a Location header field value containing
#   either the HTTP request's original Effective Request URI (see
#   Section 12 "Constructing an Effective Request URI", below) altered as
#   necessary to have a URI scheme of "https", or a URI generated
#   according to local policy (which SHOULD employ a URI scheme of
#   "https").

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "HSTS: Error code and Header"
        self.request          = "HTTP / HTTP/1.0\r\n" + \
                                "Host: %s\r\n" %(NICK)
        self.expected_error   = 301
        self.expected_content = ["https://"]

    def Prepare (self, www):
        droot = self.Mkdir (www, "%s_droot"%(NICK))

        vars = globals()
        vars.update(locals())
        self.conf = CONF %(vars)

    def Precondition (self):
        return not self.is_ssl
