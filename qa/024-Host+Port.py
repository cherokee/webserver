from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Host header with port"

        self.expected_error = 200
        self.request        = "GET / HTTP/1.1\r\n" +\
                              "Host: www.0x50.org:80\r\n"

