from base import *
from os import system

DIR  = "dir_redirect154/a/b/c"
REQ  = "%s/d/e" % (DIR)

CONF = """
vserver!default!rule!directory!/<dir>!handler = common
vserver!default!rule!directory!/<dir>!document_root = %s
vserver!default!rule!directory!/<dir>!priority = 1540
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "common-dirlist: redir to add final /"
        self.request           = "GET /%s HTTP/1.0\r\n" % (REQ)
        self.expected_error    = 301
        self.expected_content  = "Location: /%s/" % (REQ)

    def Prepare (self, www):
        dr = self.Mkdir (www, DIR)
        re = self.Mkdir (www, REQ)

        tmp = CONF % (dr)
        self.conf = tmp.replace ('<dir>', DIR)

