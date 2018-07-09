import types
import itertools
from base import *

DIR     = "flcache-expired2"
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
    def __init__ (self, filename):
        TestBase.__init__ (self, __file__)
        self.request = "GET /%s/%s HTTP/1.0\r\n" %(DIR, filename) +\
                       "Connection: close\r\n"
        self.expected_error = 200


class Test (TestCollection):
    counter = itertools.count()

    def __init__ (self):
        TestCollection.__init__ (self, __file__)

        self.name           = "Front-line: Expired Content (Epoc)"
        self.conf           = CONF
        self.proxy_suitable = True
        self.delay          = 1

    def Prepare (self, www):
        self.local_dir = self.Mkdir (www, DIR)

    def JustBefore (self, www):
        test_num = Test.counter.next()
        self.filename = "test282-id%s-test%s" %(id(self), test_num)

        # Write the new file
        self.WriteFile (self.local_dir, self.filename, 0755, CGI_CODE)

        # Create sub-request objects
        self.Empty()

        # Miss
        obj = self.Add (TestEntry (self.filename))
        obj.expected_content = [CONTENT, "X-Cache: MISS"]

        # Miss
        obj = self.Add (TestEntry (self.filename))
        obj.expected_content = [CONTENT, "X-Cache: MISS"]

    def JustAfter (self, www):
        # Clean up the local file
        fp = os.path.join (self.local_dir, self.filename)
        os.unlink (fp)
        self.filename = None

    def Precondition (self):
        return not self.is_ssl
