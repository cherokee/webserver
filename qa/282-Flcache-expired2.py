import types
from base import *

DIR     = "flcache-expired2"
FILE    = "test.cgi"
CONTENT = "Front-line checks back-end 'Expires' header"

CONF = """
vserver!1!rule!2820!match = directory
vserver!1!rule!2820!match!directory = /%(DIR)s
vserver!1!rule!2820!handler = cgi
vserver!1!rule!2820!flcache = 1
""" %(globals())

CGI_CODE = """#!/bin/sh

echo "Content-Type: text/plain"
echo "Expires: Thu, 01 Jan 1970 00:00:00 GMT"
echo

echo "%(CONTENT)s"
""" %(globals())


class TestEntry (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.request = "GET /%s/%s HTTP/1.0\r\n" %(DIR, FILE) +\
                       "Connection: close\r\n"
        self.expected_error = 200


class Test (TestCollection):
    def __init__ (self):
        TestCollection.__init__ (self, __file__)

        self.name           = "Front-line: Expired Content (Epoc)"
        self.conf           = CONF
        self.proxy_suitable = True

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0755, CGI_CODE)

        # Miss
        obj = self.Add (TestEntry())
        obj.expected_content = [CONTENT, "X-Cache: MISS"]

        # Miss
        obj = self.Add (TestEntry())
        obj.expected_content = [CONTENT, "X-Cache: MISS"]
