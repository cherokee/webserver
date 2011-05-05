import types
from base import *

DIR         = "flcache-gzip1"
FILE        = "test.cgi"
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
    def __init__ (self, use_gzip):
        TestBase.__init__ (self, __file__)
        self.request = "GET /%s/%s HTTP/1.0\r\n" %(DIR, FILE) +\
                       "Connection: close\r\n"
        if use_gzip:
            self.request += "Accept-Encoding: gzip\r\n"

        self.expected_error = 200


class Test (TestCollection):
    def __init__ (self):
        TestCollection.__init__ (self, __file__)

        self.name           = "Front-line: GZip, no-GZip, GZip, no-GZip"
        self.conf           = CONF
        self.proxy_suitable = True

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0444, CONTENT)

        # GZip (miss)
        obj = self.Add (TestEntry(True))
        obj.forbidden_content = [CONTENT]
        obj.expected_content  = [GZIP_HEADER, "X-Cache: MISS"]

        # No-GZip (miss)
        obj = self.Add (TestEntry(False))
        obj.expected_content  = [CONTENT, "X-Cache: MISS"]
        obj.forbidden_content = [GZIP_HEADER]

        # GZip (hit)
        obj = self.Add (TestEntry(True))
        obj.forbidden_content = [CONTENT]
        obj.expected_content  = [GZIP_HEADER, "X-Cache: HIT"]

        # No-GZip (hit)
        obj = self.Add (TestEntry(False))
        obj.expected_content  = [CONTENT, "X-Cache: HIT"]
        obj.forbidden_content = [GZIP_HEADER]
