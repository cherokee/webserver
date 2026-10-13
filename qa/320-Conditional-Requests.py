import os
from base import *

DIR = "conditional_requests_320"
CONTENT = "Malformed entity-tag regression.\n"
MTIME = 0x6ace4e1a

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Malformed If-None-Match is rejected"
        self.request = "GET /%(DIR)s/file HTTP/1.1\r\n" % globals() + \
                       "Host: localhost\r\n" + \
                       "If-None-Match: W/unterminated\r\n"
        self.expected_error = 400
        self.forbidden_content = CONTENT

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, "file", 0444, CONTENT)
        os.utime (f, (MTIME, MTIME))

