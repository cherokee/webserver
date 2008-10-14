from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Missing file"

        self.expected_error = 404
        self.request        = "GET /this_doesnt_exist HTTP/1.0\r\n"
