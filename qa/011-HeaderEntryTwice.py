from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Header entry twice"

        self.expected_error = 200
        self.request        = "GET / HTTP/1.0\r\n" +\
                              "Entry: value\r\n" +\
                              "Entry: value\r\n"


