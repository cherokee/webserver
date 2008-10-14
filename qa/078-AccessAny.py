from base import *
from os import system

MAGIC = "Allow From anywhere"

CONF = """
vserver!1!rule!780!match = directory
vserver!1!rule!780!match!directory = /allow_range4
vserver!1!rule!780!match!final = 0
vserver!1!rule!780!allow_from = ::/0,0.0.0.0/0
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Allow from anywhere"
        self.request           = "GET /allow_range4/file HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.conf              = CONF

    def Prepare (self, www):
        self.Mkdir (www, "allow_range4")
        self.WriteFile (www, "allow_range4/file", 0444, MAGIC)
