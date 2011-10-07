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

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "HSTS: with Subdomains"
        self.request          = "HTTP / HTTP/1.0\r\n" + \
                                "Host: %s\r\n" %(NICK)
        self.expected_error   = 301
        self.expected_content = ["Strict-Transport-Security:", "includeSubdomains"]

    def Prepare (self, www):
        droot = self.Mkdir (www, "%s_droot"%(NICK))

        vars = globals()
        vars.update(locals())
        self.conf = CONF %(vars)
