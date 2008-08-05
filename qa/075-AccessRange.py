from base import *
from os import system

MAGIC = "Allow From range"

CONF = """
vserver!1!rule!750!match = directory
vserver!1!rule!750!match!directory = /allow_range1
vserver!1!rule!750!match!final = 0
vserver!1!rule!750!allow_from = ::1/128,127.0.0.1/32
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "Allow from range"
        self.request           = "GET /allow_range1/file HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.conf              = CONF

    def Prepare (self, www):
        self.Mkdir (www, "allow_range1")
        self.WriteFile (www, "allow_range1/file", 0444, MAGIC)

