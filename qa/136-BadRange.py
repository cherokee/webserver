import random
import string
from qa_base import *
from qa_util import *

LENGTH = 100

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Bad Content Range"

        self.request           = "GET /BadRange1 HTTP/1.0\r\n" +\
                                 f"Range: bytes={int(LENGTH + 2)}-{int(LENGTH + 3)}\r\n"
        self.expected_error    = 416
        self.expected_content  = f"Content-Range: bytes */{int(LENGTH)}"

    def Prepare (self, www):
        random  = letters_random (LENGTH)
        self.WriteFile (www, "BadRange1", 0o444, random)
