import random
import string
from base import *
from util import *

MAGIC  = str_random (100)
LENGTH = 100
OFFSET = 15

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Content Range, start"

        self.request                 = "GET /Range100b HTTP/1.0\r\n" +\
                                       "Range: bytes=%d-\r\n" % (OFFSET)
        length = LENGTH - OFFSET
        self.expected_error          = 206
        self.expected_content        = [MAGIC[OFFSET:], "Content-Length: %d" % (length)]
        self.forbidden_content       = MAGIC[:OFFSET]

    def Prepare (self, www):
        self.WriteFile (www, "Range100b", 0444, MAGIC)

