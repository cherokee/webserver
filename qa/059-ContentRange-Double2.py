import random
import string
from base import *
from util import *

LENGTH  = 100*1024
OFFSET1 = 15*1024
OFFSET2 = 40*1024


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Content Range 100k, both"

        self.request           = "GET /Range100kBoth HTTP/1.0\r\n" +\
                                 "Range: bytes=%d-%d\r\n" % (OFFSET1, OFFSET2)
        self.expected_error    = 206

    def Prepare (self, www):
        srandom = str_random (LENGTH)
        self.WriteFile (www, "Range100kBoth", 0444, srandom)

        expected   = self.WriteTemp (srandom[OFFSET1:OFFSET2])
        forbidden1 = self.WriteTemp (srandom[OFFSET1:])
        forbidden2 = self.WriteTemp (srandom[:OFFSET2])

        self.expected_content  = ["file:"+expected, "Content-Length: %d" % (OFFSET2-OFFSET1 + 1)]
        self.forbidden_content = ["file:"+forbidden1, "file:"+forbidden2]
