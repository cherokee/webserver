from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Host header"

        self.expected_error = 200
        self.request        = "GET / HTTP/1.1\r\n" +\
                              "Connection: Close\r\n" + \
                              "Host: www.cherokee-project.com\r\n"

