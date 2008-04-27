import random
import string
from base import *
from util import *

DIR = "common_pathinfo_1"

CONF = """
vserver!default!rule!1490!match = directory
vserver!default!rule!1490!match!directory = <dir>
vserver!default!rule!1490!handler = common
vserver!default!rule!1490!handler!allow_pathinfo = 1
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Handler common: PathInfo"

        self.request           = "GET /%s/file/path/info HTTP/1.0\r\n" % (DIR)
        self.expected_error    = 200

    def Prepare (self, www):
        test_dir = self.Mkdir (www, DIR)
        self.conf = CONF.replace('<dir>', '/'+DIR)

        self.WriteFile (test_dir, "file", 0444)
