import random
import string
from base import *
from util import *

LENGTH = 100

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Bad Content Range"

        self.request           = "GET /BadRange1 HTTP/1.0\r\n" +\
                                 "Range: bytes=%d-%d\r\n" % (LENGTH+2 ,LENGTH+3)
        self.expected_error    = 416
        self.expected_content  = "Content-Range: bytes */%d" % (LENGTH)

    def Prepare (self, www):
        random  = letters_random (LENGTH)
        self.WriteFile (www, "BadRange1", 0444, random)

