from base import *
from os import system

MAGIC = "Allow From anywhere"

CONF = """
vserver!default!directory!/allow_range4!allow_from = ::/0,0.0.0.0/0
vserver!default!directory!/allow_range4!priority = 780
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "Allow from anywhere"
        self.request           = "GET /allow_range4/file HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.conf              = CONF

    def Prepare (self, www):
        self.Mkdir (www, "allow_range4")
        self.WriteFile (www, "allow_range4/file", 0444, MAGIC)
