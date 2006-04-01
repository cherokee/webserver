from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Redir to URL with params"

        self.request          = "GET /redir47/something?param=1&param2 HTTP/1.0\r\n"
        self.conf             = "Directory /redir47 { Handler redir { URL http://www.0x50.org } }"
        self.expected_error   = 301
