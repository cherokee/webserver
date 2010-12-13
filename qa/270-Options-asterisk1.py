from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "OPTIONS *"
        self.request           = "OPTIONS * HTTP/1.0\r\n" % (globals())
        self.expected_error    = 200
        self.expected_content  = ["Content-Length: 0"]
        self.forbidden_content = ["Bad Request"]
