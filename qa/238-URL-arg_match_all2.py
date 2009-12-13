from base import *

DIR   = "header_arg_match_all_2"
ARGS  = "first238=1&second238=http://1.1.1.1&third238=3"
MAGIC = "magic string"

CONF = """
vserver!1!rule!2380!match = and
vserver!1!rule!2380!match!left = url_arg
vserver!1!rule!2380!match!left!match = not_found
vserver!1!rule!2380!match!right = directory
vserver!1!rule!2380!match!right!directory = /%s
vserver!1!rule!2380!handler = cgi
""" % (DIR)

CGI = """#!/bin/sh

echo "Content-Type: text/plain"
echo
echo "%s"
""" % (MAGIC)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Rule url_arg: no match all"

        self.request           = "GET /%s/test?%s HTTP/1.0\r\n" % (DIR, ARGS)
        self.conf              = CONF
        self.expected_error    = 200
        self.required_content  = ["/bin/sh", "echo", MAGIC]

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, 'test', 0755, CGI)
