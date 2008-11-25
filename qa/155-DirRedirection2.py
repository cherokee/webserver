from base import *
from os import system
from os.path import join

DIR  = "dir_redirect155"
REQ  = "%s/e/f" % (DIR)
DIR2 = "%s/g/h" % (REQ)

CONF = """
vserver!1!rule!1550!match = directory
vserver!1!rule!1550!match!directory = /<dir>
vserver!1!rule!1550!handler = common
vserver!1!rule!1550!document_root = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "common-dirlist: redir to add final / 2"
        self.request           = "GET /%s HTTP/1.0\r\n" % (REQ)
        self.expected_error    = 301
        self.expected_content  = ["Location: ", "/%s/\r\n" % (REQ)]

    def Prepare (self, www):
        self.Mkdir (www, DIR2)

        tmp = CONF % (join (www,DIR))
        self.conf = tmp.replace ('<dir>', DIR)

