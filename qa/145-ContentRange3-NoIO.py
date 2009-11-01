import random
import string
from base import *
from util import *

MAGIC  = str_random (100)
LENGTH = 100
OFFSET = 15
DIR    = "range_start_100b2_noio"

CONF = """
vserver!1!rule!1450!match = directory
vserver!1!rule!1450!match!directory = <dir>
vserver!1!rule!1450!handler = file
vserver!1!rule!1450!handler!iocache = 0
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Content Range no-iocache, end"

        self.request           = "GET /%s/Range100b2 HTTP/1.0\r\n" % (DIR) +\
                                 "Range: bytes=-%d\r\n" % (OFFSET)

        self.forbidden_content = MAGIC[OFFSET:]
        self.expected_error    = 206
        self.expected_content  = [MAGIC[-OFFSET:],
                                  "Content-Length: %d" % (OFFSET),
                                  "Content-Range: bytes %d-%d/%d" % (LENGTH-OFFSET, LENGTH-1, LENGTH)]

    def Prepare (self, www):
        test_dir = self.Mkdir (www, DIR)
        self.conf = CONF.replace('<dir>', test_dir)

        self.WriteFile (test_dir, "Range100b2", 0444, MAGIC)
