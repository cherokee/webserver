from base import *

DIR   = "header_arg_match_all_1"
ARGS  = "first237=1&second237=http://1.1.1.1&third237=3"
MAGIC = "magic string"

CONF = """
vserver!1!rule!2370!match = and
vserver!1!rule!2370!match!right = url_arg
vserver!1!rule!2370!match!right!match = http://
vserver!1!rule!2370!match!left = directory
vserver!1!rule!2370!match!left!directory = /%s
vserver!1!rule!2370!handler = cgi
""" % (DIR)

CGI = """#!/bin/sh

echo "Content-Type: text/plain"
echo
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Rule url_arg: match all"

        self.request           = "GET /%s/test?%s HTTP/1.0\r\n" % (DIR, ARGS)
        self.conf              = CONF
        self.expected_error    = 200
        self.required_content  = MAGIC
        self.forbidder_content = ["/bin/sh", "echo"]

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, 'test', 0755, CGI)
