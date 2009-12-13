from base import *

DIR   = "header_arg2_match"
ARGS  = "first236=1&second236=http://1.1.1.1&third236=3"
MAGIC = "magic string"

CONF = """
vserver!1!rule!2360!match = url_arg
vserver!1!rule!2360!match!arg = second236
vserver!1!rule!2360!match!match = isnt_there
vserver!1!rule!2360!handler = cgi
"""

CGI = """#!/bin/sh

echo "Content-Type: text/plain"
echo
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Rule url_arg: no match"

        self.request           = "GET /%s/test?%s HTTP/1.0\r\n" % (DIR, ARGS)
        self.conf              = CONF
        self.expected_error    = 200
        self.required_content  = ["/bin/sh", "echo", MAGIC]

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, 'test', 0755, CGI)
