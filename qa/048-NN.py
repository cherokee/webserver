from base import *

MAGIC = "Sesamo"
CONTENT = "This is the content of " + MAGIC
HOST = "nn1"

CONF = """
vserver!480!nick = %s
vserver!480!document_root = %s
vserver!480!rule!1!match = default
vserver!480!rule!1!handler = file
vserver!480!error_handler = error_nn
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "NN"

        self.request           = "GET /Xesano HTTP/1.1\r\n" + \
                                 "Connection: Close\r\n" + \
                                 "Host: %s\r\n" %(HOST)
        self.expected_error    = 302
        self.forbidden_content = CONTENT
        self.expected_content  = "Location: /"+MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, "nn1_root")

        self.WriteFile (d, "name")
        self.WriteFile (d, MAGIC, 0444, CONTENT)
        self.WriteFile (d, "ABCD")
        self.WriteFile (d, "alobbs")

        self.conf = CONF % (HOST, d)
