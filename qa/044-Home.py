from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Invalid home"

        self.request          = "GET /~missing/file HTTP/1.0\r\n"
        self.conf             = "UserDir public_html { Directory / { Handler common }}"
        self.expected_error   = 404
