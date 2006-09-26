import random
import string
from base import *
from util import *

LENGTH = 100*1024
OFFSET = 15


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Content Range 100k, end"

        self.request           = "GET /Range100k2 HTTP/1.0\r\n" +\
                                 "Range: bytes=-%d\r\n" % (OFFSET)
        self.expected_error    = 206

    def Prepare (self, www):
        srandom = str_random (LENGTH)
        self.WriteFile (www, "Range100k2", 0444, srandom)

        forbidden = self.WriteTemp (srandom[OFFSET:])

        self.expected_content  = [srandom[:OFFSET], "Content-Length: %d" % (OFFSET + 1)]
        self.forbidden_content = "file:" + forbidden


