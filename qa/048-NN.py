from base import *

MAGIC = "Sesamo"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "NN"

        self.request          = "GET /nn1/Xesano HTTP/1.0\r\n"
        self.conf             = "Directory /nn1 { Handler nn }"
        self.expected_error   = 301
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "nn1")
        self.WriteFile (www, "nn1/name")
        self.WriteFile (www, "nn1/"+MAGIC)
        self.WriteFile (www, "nn1/ABCD")
        self.WriteFile (www, "nn1/alobbs")
