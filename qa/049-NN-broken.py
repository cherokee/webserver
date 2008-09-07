from base import *

HOST = "nn2"

CONF = """
vserver!490!nick = %s
vserver!490!document_root = %s
vserver!490!rule!1!match = default
vserver!490!rule!1!handler = file
vserver!490!error_handler = error_nn
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Broken NN"

        self.request        = "GET /missing HTTP/1.1\r\n" + \
                              "Connection: Close\r\n" + \
                              "Host: %s\r\n" % (HOST)
        self.expected_error = 404

    def Prepare (self, www):
        d = self.Mkdir (www, "nn2_root")
        self.conf = CONF % (HOST, d)
