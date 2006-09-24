import random
import string
from base import *
from util import *

MAGIC   = str_random (100)
LENGTH  = 100
OFFSET1 = 15
OFFSET2 = 40


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Content Range 100, both"

        self.request           = "GET /Range100Both HTTP/1.0\r\n" +\
                                 "Range: bytes=%d-%d\r\n" % (OFFSET1, OFFSET2)
        self.expected_error    = 206
        self.expected_content  = [MAGIC[OFFSET1:OFFSET2], "Content-Length: %d" % (OFFSET2-OFFSET1 + 1)]
        self.forbidden_content = [MAGIC[OFFSET1:], MAGIC[:OFFSET2]]

    def Prepare (self, www):
        self.WriteFile (www, "Range100Both", 0444, MAGIC)
