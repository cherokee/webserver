from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Missing file II"

        self.expected_error = 404
        self.request        = "GET /this_doesnt_exist?param=1 HTTP/1.0\r\n"
