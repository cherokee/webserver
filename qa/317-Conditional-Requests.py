import os
from base import *

DIR = "conditional_requests_317"
CONTENT = "If-Match requires a strong validator.\n"
MTIME = 0x6ace4e1a

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Weak If-Match fails strong comparison"
        self.request = "GET /%(DIR)s/file HTTP/1.1\r\n" % globals() + \
                       "Host: localhost\r\n" + \
                       "If-Match: W/\"6ace4e1a=26\"\r\n"
        self.expected_error = 412
        self.forbidden_content = CONTENT

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, "file", 0444, CONTENT)
        os.utime (f, (MTIME, MTIME))

