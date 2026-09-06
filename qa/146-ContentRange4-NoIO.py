import random
import string
from qa_base import *
from qa_util import *

LENGTH = 100*1024
OFFSET = 15
DIR    = "range_end_100k_noio"

CONF = """
vserver!1!rule!1460!match = directory
vserver!1!rule!1460!match!directory = <dir>
vserver!1!rule!1460!handler = file
vserver!1!rule!1460!handler!iocache = 0
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Content Range 100k no-iocache, end"

        self.request           = f"GET /{DIR}/Range100k2 HTTP/1.0\r\n" +\
                                 f"Range: bytes=-{int(OFFSET)}\r\n"
        self.expected_error    = 206

    def Prepare (self, www):
        test_dir = self.Mkdir (www, DIR)
        self.conf = CONF.replace('<dir>', test_dir)

        srandom = str_random (LENGTH)
        self.WriteFile (test_dir, "Range100k2", 0o444, srandom)

        forbidden = self.WriteTemp (srandom[OFFSET:])

        self.forbidden_content = "file:" + forbidden
        self.expected_content  = [srandom[-OFFSET:],
                                  f"Content-Length: {int(OFFSET)}",
                                  f"Content-Range: bytes {int(LENGTH - OFFSET)}-{int(LENGTH - 1)}/{int(LENGTH)}"]
