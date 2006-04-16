from base import *

MAGIC = "Sesamo"

CONF = """
vserver!default!directory!/nn1!handler = nn
vserver!default!directory!/nn1!priority = 480
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "NN"

        self.request          = "GET /nn1/Xesano HTTP/1.0\r\n"
        self.conf             = CONF
        self.expected_error   = 301
        self.expected_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "nn1")
        self.WriteFile (www, "nn1/name")
        self.WriteFile (www, "nn1/"+MAGIC)
        self.WriteFile (www, "nn1/ABCD")
        self.WriteFile (www, "nn1/alobbs")
