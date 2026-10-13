import os
from base import *

DIR = "conditional_requests_318"
CONTENT = "Multiple entity-tag list regression.\n"
MTIME = 0x6ace4e1a
ETAG = '6ace4e1a=%x' % len(CONTENT)

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "If-None-Match parses multiple entity-tags"
        self.request = "GET /%(DIR)s/file HTTP/1.1\r\n" % globals() + \
                       "Host: localhost\r\n" + \
                       "If-None-Match: \"different\", W/\"%(ETAG)s\"\r\n" % globals()
        self.expected_error = 304
        self.forbidden_content = CONTENT

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, "file", 0444, CONTENT)
        os.utime (f, (MTIME, MTIME))

