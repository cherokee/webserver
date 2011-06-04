from base import *

DIR     = "flcache-cookie1"
FILE    = "test.cgi274"
CONTENT = "Front-line does not cache responses setting cookies"

CONF = """
vserver!1!rule!2760!match = directory
vserver!1!rule!2760!match!directory = /%(DIR)s
vserver!1!rule!2760!handler = cgi
vserver!1!rule!2760!flcache = 1
""" %(globals())


CGI_CODE = """#!/bin/sh

echo "Content-Type: text/plain"
echo "Set-cookie: foo=bar"
echo

echo "%(CONTENT)s"
""" %(globals())



class TestEntry (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.request        = "GET /%s/%s HTTP/1.0\r\n" %(DIR, FILE) +\
                              "Connection: close\r\n"
        self.expected_error = 200


class Test (TestCollection):
    def __init__ (self):
        TestCollection.__init__ (self, __file__)

        self.name           = "Front-line cache: Cookie"
        self.conf           = CONF
        self.proxy_suitable = True
        self.delay          = 1

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0755, CGI_CODE)

        # First request
        obj = self.Add (TestEntry())
        obj.expected_content = ['X-Cache: MISS', CONTENT]

        # Second request
        obj = self.Add (TestEntry())
        obj.expected_content = ['X-Cache: MISS', CONTENT]
