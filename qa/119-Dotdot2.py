from base import *

MAGIC = "Cherokee accepts dot dot inside the request"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Dot dot reduction"
        self.request           = "GET /dotdot2/aa/bb/cc/../../dd/../../dir/file HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, "dotdot2/dir")
        self.WriteFile (d, "file", 0444, MAGIC);
