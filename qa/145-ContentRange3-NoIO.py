import random
import string
from qa_base import *
from qa_util import *

MAGIC  = str_random (100)
LENGTH = 100
OFFSET = 15
DIR    = "range_start_100b2_noio"

CONF = """
vserver!1!rule!1450!match = directory
vserver!1!rule!1450!match!directory = <dir>
vserver!1!rule!1450!handler = file
vserver!1!rule!1450!handler!iocache = 0
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Content Range no-iocache, end"

        self.request           = f"GET /{DIR}/Range100b2 HTTP/1.0\r\n" +\
                                 f"Range: bytes=-{int(OFFSET)}\r\n"

        self.forbidden_content = MAGIC[OFFSET:]
        self.expected_error    = 206
        self.expected_content  = [MAGIC[-OFFSET:],
                                  f"Content-Length: {int(OFFSET)}",
                                  f"Content-Range: bytes {int(LENGTH - OFFSET)}-{int(LENGTH - 1)}/{int(LENGTH)}"]

    def Prepare (self, www):
        test_dir = self.Mkdir (www, DIR)
        self.conf = CONF.replace('<dir>', test_dir)

        self.WriteFile (test_dir, "Range100b2", 0o444, MAGIC)
