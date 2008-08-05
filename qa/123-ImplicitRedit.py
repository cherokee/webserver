from base import *

TEST_FILE = "Cherokee is the fastest one"

CONF = """        
vserver!1!rule!1230!match = directory
vserver!1!rule!1230!match!directory = /implicit_redir1
vserver!1!rule!1230!handler = dirlist
vserver!1!rule!1230!document_root = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Implicit Redir"

        self.request           = "GET /implicit_redir1 HTTP/1.0\r\n"
        self.expected_error    = 301
        self.expected_content  = "Location: /implicit_redir1/"

    def Prepare (self, www):
        d  = self.Mkdir (www, "implicit_redir1_DIR")
        self.WriteFile (d, TEST_FILE, 0444, "");

        self.conf = CONF % (d)
