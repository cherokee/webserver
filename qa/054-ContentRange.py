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
        self.name = "Content Range, start"

        self.request                 = "GET /Range100b HTTP/1.0\r\n" +\
                                       f"Range: bytes={int(OFFSET)}-\r\n"
        length = LENGTH - OFFSET
        self.expected_error          = 206
        self.expected_content        = [MAGIC[OFFSET:], f"Content-Length: {int(length)}"]
        self.forbidden_content       = MAGIC[:OFFSET]

    def Prepare (self, www):
        self.WriteFile (www, "Range100b", 0o444, MAGIC)
