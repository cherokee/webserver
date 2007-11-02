import random
import string
from base import *
from util import *

DIR = "common_pathinfo_2"

CONF = """
vserver!default!directory!<dir>!handler = common
vserver!default!directory!<dir>!handler!allow_pathinfo = 0
vserver!default!directory!<dir>!priority = 1500
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Handler common: No PathInfo"

        self.request           = "GET /%s/file/path/info HTTP/1.0\r\n" % (DIR)
        self.expected_error    = 404

    def Prepare (self, www):
        test_dir = self.Mkdir (www, DIR)
        self.conf = CONF.replace('<dir>', '/'+DIR)

        self.WriteFile (test_dir, "file", 0444)
