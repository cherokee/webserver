import random
import string
from qa_base import *
from qa_util import *

MAGIC  = str_random (100)
LENGTH = 100
OFFSET = 15


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Content Range, end"

        self.request           = "GET /Range100b2 HTTP/1.0\r\n" +\
                                 f"Range: bytes=-{int(OFFSET)}\r\n"
        self.expected_error    = 206
        self.expected_content  = [MAGIC[-OFFSET:],
                                  f"Content-Length: {int(OFFSET)}",
                                  f"Content-Range: bytes {int(LENGTH - OFFSET)}-{int(LENGTH - 1)}/{int(LENGTH)}"]
        self.forbidden_content = MAGIC[OFFSET:]

    def Prepare (self, www):
        self.WriteFile (www, "Range100b2", 0o444, MAGIC)
