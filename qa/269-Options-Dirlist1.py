from base import *

DIR = "269_options_dirlist"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "OPTIONS: Dirlist request"
        self.request           = "OPTIONS /%(DIR)s/ HTTP/1.0\r\n" % (globals())
        self.expected_error    = 200
        self.expected_content  = ["Allow: ", "GET", "OPTIONS", ", ", "Content-Length: 0"]
        self.forbidden_content = ["Index of"]

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
