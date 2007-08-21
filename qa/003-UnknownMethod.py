from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Unknown Method"

        self.expected_error = 501
        self.request        = "XYZ / HTTP/1.0\r\n" +\
                              "Connection: Keep-alive\r\n"

