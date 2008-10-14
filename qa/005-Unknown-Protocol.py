from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Unknown Method"

        self.expected_error = 400
        self.request        = "GET / CHEROKEE/1.0\r\n"

