import os
from base import *

DIR = "conditional_requests_316"
MARKER = "conditional request date must not override ETag\n"
SIZE = 588454
MTIME = 0x6ace4e1a
DATE = "Tue, 13 Oct 2026 15:28:26 GMT"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Nonmatching If-None-Match prevents If-Modified-Since 304"
        self.request = "GET /%(DIR)s/file HTTP/1.1\r\n" % globals() + \
                       "Host: localhost\r\n" + \
                       "If-Modified-Since: %(DATE)s\r\n" % globals() + \
                       "If-None-Match: \"different\"\r\n"
        self.expected_error = 200
        self.expected_content = MARKER

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        content = (MARKER * ((SIZE / len(MARKER)) + 1))[:SIZE]
        f = self.WriteFile (d, "file", 0444, content)
        os.utime (f, (MTIME, MTIME))

