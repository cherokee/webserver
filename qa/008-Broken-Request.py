from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Broken request"

        self.expected_error = 400
        self.request        = "GET / HTTP/1.0 thing\r\n"

