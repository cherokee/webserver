from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Directory index redir"

        self.expected_error = 301
        self.request        = "GET /redir1 HTTP/1.0\r\n"

    def Prepare (self, www):
        self.Mkdir (www, "redir1")
