from base import *

DIR = "cgi-bin_serveraddr"

CONF = """
vserver!1!rule!1950!match = directory
vserver!1!rule!1950!match!directory = /%s
vserver!1!rule!1950!handler = cgi
""" % (DIR)

CGI_BASE = """#!/bin/sh
echo "Content-Type: text/plain"
echo
echo "SERVER_ADDR: $SERVER_ADDR"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI: SERVER_ADDR"

        self.request               = "GET /%s/test HTTP/1.0\r\n" % (DIR)
        self.conf                  = CONF
        self.expected_one_of_these = ["127.0.0.1", "::1"]
        self.expected_error        = 200
        self.proxy_suitable        = False

    def Prepare (self, www):
        self.Mkdir (www, DIR)
        self.WriteFile (www, "%s/test"%(DIR), 0755, CGI_BASE)
