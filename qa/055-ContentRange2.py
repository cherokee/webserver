import random
import string
from base import *
from util import *

LENGTH = 100*1024
OFFSET = 15


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Content Range 100k, start"

        self.request           = "GET /Range100k HTTP/1.0\r\n" +\
                                 "Range: bytes=%d-\r\n" % (OFFSET)
        self.expected_error    = 206

    def Prepare (self, www):
        random  = letters_random (LENGTH)
        self.WriteFile (www, "Range100k", 0444, random)

        tmpfile = self.WriteTemp (random[OFFSET:])

        self.expected_content  = ["file:"+tmpfile, "Content-Length: %d" % (LENGTH-OFFSET)]
        self.forbidden_content = random[:OFFSET]


