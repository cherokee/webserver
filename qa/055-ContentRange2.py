import string
from qa_base import *
from qa_util import *

LENGTH = 100*1024
OFFSET = 15


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Content Range 100k, start"

        self.request           = "GET /Range100k HTTP/1.0\r\n" +\
                                 f"Range: bytes={int(OFFSET)}-\r\n"
        self.expected_error    = 206

    def Prepare (self, www):
        tmp = letters_random (LENGTH)
        self.WriteFile (www, "Range100k", 0o444, tmp)

        tmpfile = self.WriteTemp (tmp[OFFSET:])

        self.expected_content  = ["file:"+tmpfile, f"Content-Length: {int(LENGTH - OFFSET)}"]
        self.forbidden_content = tmp[:OFFSET]

