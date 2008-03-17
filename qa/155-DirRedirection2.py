from base import *
from os import system
from os.path import join

DIR  = "dir_redirect155"
REQ  = "%s/e/f" % (DIR)
DIR2 = "%s/g/h" % (REQ)

CONF = """
vserver!default!directory!/<dir>!handler = common
vserver!default!directory!/<dir>!document_root = %s
vserver!default!directory!/<dir>!priority = 1550
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name              = "common -> dirlist: redir to add final / 2"
        self.request           = "GET /%s HTTP/1.0\r\n" % (REQ)
        self.expected_error    = 301
        self.expected_content  = "Location: /%s/" % (REQ)

    def Prepare (self, www):
        self.Mkdir (www, DIR2)

        tmp = CONF % (join (www,DIR))
        self.conf = tmp.replace ('<dir>', DIR)

