from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Missing Protocol"

        self.expected_error = 400
        self.request        = "GET / \r\n" +\
                              "Connection: Keep-alive\r\n"

