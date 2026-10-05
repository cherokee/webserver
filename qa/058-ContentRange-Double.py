import random
import string
from qa_base import *
from qa_util import *

MAGIC   = str_random (100)
OFFSET1 = 15
OFFSET2 = 40


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Content Range 100, both"

        self.request           = "GET /Range100Both HTTP/1.0\r\n" +\
                                 f"Range: bytes={int(OFFSET1)}-{int(OFFSET2)}\r\n"
        self.expected_error    = 206
        self.expected_content  = [MAGIC[OFFSET1:OFFSET2+1],
                                  f"Content-Length: {int(OFFSET2 - OFFSET1 + 1)}"]
        self.forbidden_content = [MAGIC[:OFFSET1], MAGIC[:OFFSET2]]

    def CustomTest (self):
        body = self.reply.split ("\r\n\r\n")[1]
        if len(body) != OFFSET2-OFFSET1 + 1:
            return -1

    def Prepare (self, www):
        self.WriteFile (www, "Range100Both", 0o444, MAGIC)
