from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Basic"

        self.expected_error = 200
        self.request        = "GET /basic_test1/ HTTP/1.0\r\n"

    def Prepare (self, www):
        self.Mkdir (www, "basic_test1")

