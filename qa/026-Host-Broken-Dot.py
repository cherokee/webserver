from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Broken Host header II"

        self.expected_error = 400
        self.request        = "GET / HTTP/1.1\r\n" +\
                              "Connection: Close\r\n" + \
                              "Host: .hostname.domain\r\n"
