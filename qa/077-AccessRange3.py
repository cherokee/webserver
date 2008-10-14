from base import *
from os import system

MAGIC = "Allow From range invalid"

CONF = """
vserver!1!rule!770!match = directory
vserver!1!rule!770!match!directory = /allow_range3
vserver!1!rule!770!match!final = 0
vserver!1!rule!770!allow_from = 123.123.0.0/16,0102::/16
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Allow from invalid range"
        self.request           = "GET /allow_range3/file HTTP/1.0\r\n"
        self.expected_error    = 403
        self.forbidden_content = MAGIC
        self.conf              = CONF

    def Prepare (self, www):
        self.Mkdir (www, "allow_range3")
        self.WriteFile (www, "allow_range3/file", 0444, MAGIC)
