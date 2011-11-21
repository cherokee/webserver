from base import *

NICK = "test-294"

CONF = """
vserver!2940!nick = %(NICK)s
vserver!2940!document_root = %(droot)s
vserver!2940!hsts = 1
vserver!2940!hsts!subdomains = 0
vserver!2940!rule!1!match = default
vserver!2940!rule!1!handler = dirlist
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "HSTS over SSL: without Subdomains"
        self.request           = "GET / HTTP/1.0\r\n" + \
                                 "Host: %s\r\n" %(NICK)
        self.expected_error    = 200
        self.expected_content  = ["Strict-Transport-Security:"]
        self.forbidden_content = ["includeSubdomains"]

    def Prepare (self, www):
        droot = self.Mkdir (www, "%s_droot"%(NICK))

        vars = globals()
        vars.update(locals())
        self.conf = CONF %(vars)

    def Precondition (self):
        return self.is_ssl
