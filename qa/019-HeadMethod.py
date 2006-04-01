from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Head Method"

        self.expected_error = 200
        self.request        = "HEAD / HTTP/1.0\r\n" +\
                              "Connection: Keep-alive\r\n"

