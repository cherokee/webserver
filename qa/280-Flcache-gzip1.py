import types
import itertools
from base import *

DIR         = "flcache-gzip1"
CONTENT     = "Front-line supports encoded and non-encoded content"
GZIP_HEADER = "\037\213"

CONF = """
vserver!1!rule!2800!match = directory
vserver!1!rule!2800!match!directory = /%(DIR)s
vserver!1!rule!2800!handler = file
vserver!1!rule!2800!flcache = 1
vserver!1!rule!2800!flcache!policy = all_but_forbidden
vserver!1!rule!2800!encoder!gzip = 1
""" %(globals())


class TestEntry (TestBase):
    def __init__ (self, filename, use_gzip):
        TestBase.__init__ (self, __file__)
        self.request = "GET /%s/%s HTTP/1.0\r\n" %(DIR, filename) +\
                       "Connection: close\r\n"
        if use_gzip:
            self.request += "Accept-Encoding: gzip\r\n"

        self.expected_error = 200


class Test (TestCollection):
    counter = itertools.count()

    def __init__ (self):
        TestCollection.__init__ (self, __file__)

        self.name           = "Front-line: GZip, no-GZip, GZip, no-GZip"
        self.conf           = CONF
        self.proxy_suitable = True
        self.delay          = 1

    def JustBefore (self, www):
        test_num = Test.counter.next()
        self.filename = "test280-id%s-test%s" %(id(self), test_num)

        # Write the new file
        self.WriteFile (self.local_dir, self.filename, 0444, CONTENT)

        # Create sub-request objects
        self.Empty()

        # GZip (miss)
        obj = self.Add (TestEntry (self.filename, True))
        obj.forbidden_content = [CONTENT]
        obj.expected_content  = [GZIP_HEADER, "X-Cache: MISS"]

        # No-GZip (miss)
        obj = self.Add (TestEntry (self.filename, False))
        obj.expected_content  = [CONTENT, "X-Cache: MISS"]
        obj.forbidden_content = [GZIP_HEADER]

        # GZip (hit)
        obj = self.Add (TestEntry (self.filename, True))
        obj.forbidden_content = [CONTENT]
        obj.expected_content  = [GZIP_HEADER, "X-Cache: HIT"]

        # No-GZip (hit)
        obj = self.Add (TestEntry (self.filename, False))
        obj.expected_content  = [CONTENT, "X-Cache: HIT"]
        obj.forbidden_content = [GZIP_HEADER]

    def JustAfter (self, www):
        # Clean up the local file
        fp = os.path.join (self.local_dir, self.filename)
        os.unlink (fp)
        self.filename = None

    def Prepare (self, www):
        self.local_dir = self.Mkdir (www, DIR)

    def Precondition (self):
        return not self.is_ssl
