from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Unknown Method"

        self.expected_error = 405 # Method Not Allowed
        self.request        = "XYZ / HTTP/1.0\r\n" +\
                              "Connection: Keep-alive\r\n"

