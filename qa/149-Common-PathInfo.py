import random
import string
from qa_base import *
from qa_util import *

DIR = "common_pathinfo_1"

CONF = """
vserver!1!rule!1490!match = directory
vserver!1!rule!1490!match!directory = <dir>
vserver!1!rule!1490!handler = common
vserver!1!rule!1490!handler!allow_pathinfo = 1
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Handler common: PathInfo"

        self.request           = f"GET /{DIR}/file/path/info HTTP/1.0\r\n"
        self.expected_error    = 200

    def Prepare (self, www):
        test_dir = self.Mkdir (www, DIR)
        self.conf = CONF.replace('<dir>', '/'+DIR)

        self.WriteFile (test_dir, "file", 0o444)
