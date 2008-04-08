from base import *
from os import system

MAGIC = "Allow From range 2"

CONF = """
vserver!default!rule!directory!/allow_range2!allow_from = 127.0.0.1/8
vserver!default!rule!directory!/allow_range2!priority = 760
vserver!default!rule!directory!/allow_range2!final = 0
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

