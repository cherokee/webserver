import random
import string
from base import *
from util import *

LENGTH  = 100*1024
OFFSET1 = 15*1024
OFFSET2 = 40*1024
DIR     = "range_both_100k_noio"

CONF = """
vserver!1!rule!1480!match = directory
vserver!1!rule!1480!match!directory = <dir>
vserver!1!rule!1480!handler = file
vserver!1!rule!1480!handler!iocache = 0
"""


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Content Range 100k no-iocache, both"

        self.request           = "GET /%s/Range100kBoth HTTP/1.0\r\n" % (DIR) +\
                                 "Range: bytes=%d-%d\r\n" % (OFFSET1, OFFSET2)
        self.expected_error    = 206

    def Prepare (self, www):
        test_dir = self.Mkdir (www, DIR)
        self.conf = CONF.replace('<dir>', test_dir)

        srandom = str_random (LENGTH)
        self.WriteFile (test_dir, "Range100kBoth", 0444, srandom)

        expected   = self.WriteTemp (srandom[OFFSET1:OFFSET2])
        forbidden1 = self.WriteTemp (srandom[OFFSET1:])
        forbidden2 = self.WriteTemp (srandom[:OFFSET2])

        self.expected_content  = ["file:"+expected, "Content-Length: %d" % (OFFSET2-OFFSET1 + 1)]
        self.forbidden_content = ["file:"+forbidden1, "file:"+forbidden2]
