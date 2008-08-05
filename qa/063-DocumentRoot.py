from base import *

MAGIC="This is the magic string for the DocumentRoot test - common"

CONF = """
vserver!1!rule!630!match = directory
vserver!1!rule!630!match!directory = /droot
vserver!1!rule!630!handler = common
vserver!1!rule!630!document_root = %s
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "DocumentRoot, common"
        self.request          = "GET /droot/file HTTP/1.0\r\n"
        self.expected_error   = 200
        self.expected_content = MAGIC

    def Prepare (self, www):
        d = self.Mkdir (www, "documentroot/subdir/subdir")
        self.WriteFile (d, "file", 0444, MAGIC)

        self.conf = CONF % (d)

