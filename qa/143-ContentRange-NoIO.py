import random
import string
from base import *
from util import *

MAGIC  = str_random (100)
LENGTH = 100
OFFSET = 15
DIR    = "range_start_noio"

CONF = """
vserver!1!rule!1430!match = directory
vserver!1!rule!1430!match!directory = <dir>
vserver!1!rule!1430!match!handler = file
vserver!1!rule!1430!match!handler!iocache = 0
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Content Range no-iocache, start"

        self.request                 = "GET /%s/Range100b HTTP/1.0\r\n" % (DIR) + \
                                       "Range: bytes=%d-\r\n" % (OFFSET)
        length = LENGTH - OFFSET
        self.expected_error          = 206
        self.expected_content        = [MAGIC[OFFSET:], "Content-Length: %d" % (length)]
        self.forbidden_content       = MAGIC[:OFFSET]

    def Prepare (self, www):
        test_dir = self.Mkdir (www, DIR)
        self.WriteFile (test_dir, "Range100b", 0444, MAGIC)

        self.conf = CONF.replace('<dir>', test_dir)
