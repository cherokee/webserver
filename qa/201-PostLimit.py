from base import *

HOST  = "201_test"
POST  = "Alvaro: alobbs.com"

CONF = """
vserver!201!nick = %s
vserver!201!post_max_len = 2
vserver!201!document_root = %s

vserver!201!rule!100!match = default
vserver!201!rule!100!match!final = 1
vserver!201!rule!100!handler = cgi
"""

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo
echo "foo bar"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "POST upload limit"
        self.request           = "POST /program.cgi HTTP/1.0\r\n" + \
                                 "Host: %s\r\n" % (HOST) + \
                                 "Content-type: application/x-www-form-urlencoded\r\n" + \
                                 "Content-length: %d\r\n" % (len(POST)) + \
                                 "Connection: Close\r\n"
        self.post              = POST
        self.expected_error    = 413
        self.forbidden_content = ["/bin/sh", "foo bar"]

    def Prepare (self, www):
        d = self.Mkdir (www, HOST)
        self.WriteFile (d, "program.cgi", 0555, CGI_BASE)

        self.conf = CONF % (HOST, d)
