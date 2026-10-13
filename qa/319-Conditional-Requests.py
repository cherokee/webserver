import os
from base import *

DIR = "conditional_requests_319"
CONTENT = "If-Unmodified-Since regression.\n"
MTIME = 0x6ace4e1a

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "If-Unmodified-Since failure returns 412"
        self.request = "GET /%(DIR)s/file HTTP/1.1\r\n" % globals() + \
                       "Host: localhost\r\n" + \
                       "If-Unmodified-Since: Tue, 13 Oct 2026 15:28:25 GMT\r\n"
        self.expected_error = 412
        self.forbidden_content = CONTENT

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, "file", 0444, CONTENT)
        os.utime (f, (MTIME, MTIME))

