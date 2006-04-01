from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Broken NN"

        self.request          = "GET /missing/ HTTP/1.0\r\n"
        self.conf             = "Directory /missing { Handler nn }"
        self.expected_error   = 404

