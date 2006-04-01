import random
import string
from base import *
from util import *

LENGTH = 10*1024


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "10k random"

        self.request          = "GET /10k HTTP/1.0\r\n"
        self.expected_error   = 200

    def Prepare (self, www):
        f = self.WriteFile (www, "10k", 0444, str_random (LENGTH))
        self.expected_content = "file:" + f



