import random
import string
from qa_base import *
from qa_util import *

LENGTH = 100*1024
OFFSET = 15


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Content Range 100k, end"

        self.request           = "GET /Range100k2 HTTP/1.0\r\n" +\
                                 f"Range: bytes=-{int(OFFSET)}\r\n"
        self.expected_error    = 206

    def Prepare (self, www):
        srandom = str_random (LENGTH)
        self.WriteFile (www, "Range100k2", 0o444, srandom)

        forbidden = self.WriteTemp (srandom[OFFSET:])

        self.expected_content  = [srandom[-OFFSET:],
                                  f"Content-Length: {int(OFFSET)}",
                                  f"Content-Range: bytes {int(LENGTH - OFFSET)}-{int(LENGTH - 1)}/{int(LENGTH)}"]
        self.forbidden_content = "file:" + forbidden

