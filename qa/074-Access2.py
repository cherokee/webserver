from base import *

MAGIC = "Allow From test 2"

CONF = """
vserver!1!rule!740!match = directory
vserver!1!rule!740!match!directory = /allow2
vserver!1!rule!740!match!final = 0
vserver!1!rule!740!allow_from = 127.0.0.1,::1
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Allow from test, allowed"
        self.request           = "GET /allow2/file HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.conf              = CONF
        self.proxy_suitable    = False

    def Prepare (self, www):
        self.Mkdir (www, "allow2")
        self.WriteFile (www, "allow2/file", 0444, MAGIC)

