from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Broken Host header"

        self.expected_error = 400
        self.request        = "GET / HTTP/1.1\r\n" +\
                              "Host: :80\r\n"

