from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name    = "Connection: TE, close"
        self.request = "GET / HTTP/1.1\r\n" +\
                       "Connection: TE, close\r\n" + \
                       "Host: www.cherokee-project.com\r\n"

        self.expected_error    = 200
        self.expected_content  = ['Connection: close']
        self.forbidden_content = ['keep-alive']
