import random
import string
from base import *
from util import *

MAGIC   = str_random (100)
LENGTH  = 100
OFFSET1 = 15
OFFSET2 = 40
DIR     = "range_both_100b_noio"

CONF = """
vserver!1!rule!1470!match = directory
vserver!1!rule!1470!match!directory = <dir>
vserver!1!rule!1470!handler = file
vserver!1!rule!1470!handler!iocache = 0
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Content Range 100 no-iocache, both"

        self.request           = "GET /%s/Range100Both HTTP/1.0\r\n" % (DIR) +\
                                 "Range: bytes=%d-%d\r\n" % (OFFSET1, OFFSET2)
        self.expected_error    = 206
        self.expected_content  = [MAGIC[OFFSET1:OFFSET2], "Content-Length: %d" % (OFFSET2-OFFSET1 + 1)]
        self.forbidden_content = [MAGIC[OFFSET1:], MAGIC[:OFFSET2]]

    def Prepare (self, www):
        test_dir = self.Mkdir (www, DIR)
        self.conf = CONF.replace('<dir>', test_dir)

        self.WriteFile (test_dir, "Range100Both", 0444, MAGIC)
