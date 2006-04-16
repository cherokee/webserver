from base import *

MAGIC = "Allow From test"

CONF = """
vserver!default!directory!/allow1!allow_from = 99.99.99.99
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "Allow from test, forbidden"
        self.request           = "GET /allow1/file HTTP/1.0\r\n"
        self.expected_error    = 403
        self.forbidden_content = MAGIC
        self.conf              = CONF
        a = """
              Directory /allow1/ {
                 Handler file
                 Allow from 99.99.99.99
              }
              """

    def Prepare (self, www):
        self.Mkdir (www, "allow1")
        self.WriteFile (www, "allow1/file", 0444, MAGIC)

