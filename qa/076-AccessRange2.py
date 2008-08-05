from base import *
from os import system

MAGIC = "Allow From range 2"

CONF = """
vserver!1!rule!760!match = directory
vserver!1!rule!760!match!directory = /allow_range2
vserver!1!rule!760!match!final = 0
vserver!1!rule!760!allow_from = 127.0.0.1/8
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "Allow from range 2"
        self.request           = "GET /allow_range2/file HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.conf              = CONF

    def Prepare (self, www):
        self.Mkdir (www, "allow_range2")
        self.WriteFile (www, "allow_range2/file", 0444, MAGIC)

