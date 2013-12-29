from base import *

MAGIC = '" "qa"="xss" "other"="'

CONF = """
vserver!1!rule!3020!match = directory
vserver!1!rule!3020!match!directory = /index3
vserver!1!rule!3020!handler = dirlist
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Directory index, prevert XSS in dirlist"

        self.request           = "GET /index3/ HTTP/1.0\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.forbidden_content = MAGIC

    def Prepare (self, www):
        self.Mkdir (www, "index3")
        self.Mkdir (www, "index3/dir1")
        self.Mkdir (www, "index3/"+MAGIC)
        self.Mkdir (www, "index3/dir2")

