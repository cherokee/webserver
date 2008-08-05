import random
import string
from base import *
from util import *

LENGTH = 100*1024
OFFSET = 15
DIR    = "range_start_100k_noio"

CONF = """
vserver!1!rule!1440!match = directory
vserver!1!rule!1440!match!directory = <dir>
vserver!1!rule!1440!handler = file
vserver!1!rule!1440!handler!iocache = 0
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Content Range no-iocache 100k, start"

        self.request           = "GET /%s/Range100k HTTP/1.0\r\n" % (DIR) +\
                                 "Range: bytes=%d-\r\n" % (OFFSET)
        self.expected_error    = 206

    def Prepare (self, www):
        test_dir = self.Mkdir (www, DIR)
        self.conf = CONF.replace('<dir>', test_dir)

        random  = letters_random (LENGTH)
        self.WriteFile (test_dir, "Range100k", 0444, random)

        tmpfile = self.WriteTemp (random[OFFSET:])

        self.expected_content  = ["file:"+tmpfile, "Content-Length: %d" % (LENGTH-OFFSET)]
        self.forbidden_content = random[:OFFSET]
