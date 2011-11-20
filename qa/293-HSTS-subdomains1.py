from base import *

NICK = "test-293"

CONF = """
vserver!2930!nick = %(NICK)s
vserver!2930!document_root = %(droot)s
vserver!2930!hsts = 1
vserver!2930!hsts!subdomains = 1
vserver!2930!rule!1!match = default
vserver!2930!rule!1!handler = dirlist
"""

# 6.1.  HTTP-over-Secure-Transport Request Type
#
#   When replying to an HTTP request that was conveyed over a secure
#   transport, a HSTS Host SHOULD include in its response message a
#   Strict-Transport-Security HTTP Response Header that MUST satisfy the
#   grammar specified above in Section 5.1 "Strict-Transport-Security
#   HTTP Response Header Field".  If a Strict-Transport-Security HTTP
#   Response Header is included, the HSTS Host MUST include only one such
#   header.

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "HSTS over SSL"
        self.request          = "GET / HTTP/1.0\r\n" + \
                                "Host: %s\r\n" %(NICK)
        self.expected_error   = 200
        self.expected_content = ["Strict-Transport-Security:", "includeSubdomains"]

    def Prepare (self, www):
        droot = self.Mkdir (www, "%s_droot"%(NICK))

        vars = globals()
        vars.update(locals())
        self.conf = CONF %(vars)

    def Precondition (self):
        return self.is_ssl
