from base import *

DIR     = "flcache-replace1"
FILE    = "file.txt"
CONTENT = "Front-line cache boosts Cherokee performance"

CONF = """
vserver!1!rule!2850!match = directory
vserver!1!rule!2850!match!directory = /%(DIR)s
vserver!1!rule!2850!flcache = allow
vserver!1!rule!2850!flcache!policy = all_but_forbidden
vserver!1!rule!2850!expiration = time
vserver!1!rule!2850!expiration!caching = public
vserver!1!rule!2850!expiration!time = 300
vserver!1!rule!2850!handler = cgi
""" %(globals())

CGI_CODE = """#!/bin/sh

echo "Content-Type: text/plain"
echo "Cache-Control: no-store, no-cache"
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

        self.name           = "Front-line: Overwrite Cache-Control"
        self.conf           = CONF
        self.proxy_suitable = True
        self.delay          = 1

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0755, CGI_CODE)

        # First request
        obj = self.Add (TestEntry())
        obj.expected_content  = ['X-Cache: MISS', CONTENT, "public"]
        obj.forbidden_content = ['no-store', 'no-cache']

        # Second request
        obj = self.Add (TestEntry())
        obj.expected_content  = ['X-Cache: HIT', CONTENT, "public"]
        obj.forbidden_content = ['no-store', 'no-cache']
