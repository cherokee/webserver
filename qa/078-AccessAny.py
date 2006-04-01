from base import *
from os import system

MAGIC = "Allow From anywhere"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "Allow from anywhere"
        self.request           = "GET /allow_range4/file HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = MAGIC
        self.conf              = """
              Directory /allow_range4/ {
                 Handler file
                 Allow from ::/0, 0.0.0.0/0
              }
              """

    def Prepare (self, www):
        self.Mkdir (www, "allow_range4")
        self.WriteFile (www, "allow_range4/file", 0444, MAGIC)
