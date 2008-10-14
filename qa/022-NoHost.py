from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "HTTP 1.1 without Host"

        self.expected_error = 400
        self.request        = "GET / HTTP/1.1\r\n" +\
                              "Connection: Close\r\n"

