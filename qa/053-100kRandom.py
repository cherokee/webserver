import random
import string
from qa_base import *
from qa_util import *

LENGTH = 100*1024


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "100k random"

        self.request          = "GET /100k HTTP/1.0\r\n"
        self.expected_error   = 200

    def Prepare (self, www):
        f = self.WriteFile (www, "100k", 0o444, str_random (LENGTH))
        self.expected_content = "file:" + f
