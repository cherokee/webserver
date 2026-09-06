from qa_base import *

DIR  = "dir_redirect154/a/b/c"
REQ  = f"{DIR}/d/e"

CONF = """
vserver!1!rule!1540!match = directory
vserver!1!rule!1540!match!directory = /<dir>
vserver!1!rule!1540!handler = common
vserver!1!rule!1540!document_root = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "common-dirlist: redir to add final /"
        self.request           = f"GET /{REQ} HTTP/1.0\r\n"
        self.expected_error    = 301
        self.expected_content  = ["Location: ", f"/{REQ}/\r\n"]

    def Prepare (self, www):
        dr = self.Mkdir (www, DIR)
        re = self.Mkdir (www, REQ)

        tmp = CONF % (dr)
        self.conf = tmp.replace ('<dir>', DIR)
