from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Wrong Protocol"

        self.expected_error = 505
        self.request        = "GET / HTTP/1.3\r\n"

