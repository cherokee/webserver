from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Double dot"

        self.request        = "GET /whatever/../ HTTP/1.0\r\n"
        self.expected_error = 200
