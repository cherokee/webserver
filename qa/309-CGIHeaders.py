from base import *

MAGIC = "This is the magic string"
CONF = """
vserver!1!rule!3090!match = directory
vserver!1!rule!3090!match!directory = /cgi-bin309
vserver!1!rule!3090!handler = cgi
"""

CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo
echo "%s"
""" % (MAGIC)

# One more than defined in cherokee/handler_cgi.h
ENV_VAR_NUM = 80

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI headers exceed ENV_VAR_NUM"

        headers = "\r\n".join(['Content-Header%d: %d' % (n, n) for n in range(0, ENV_VAR_NUM)])

        self.request          = "GET /cgi-bin309/test HTTP/1.0\r\n" + headers + "\r\n"
        self.conf             = CONF
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "cgi-bin309")
        self.WriteFile (www, "cgi-bin309/test", 0755, CGI_BASE)
