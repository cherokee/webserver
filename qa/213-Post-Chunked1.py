import random
import string
from base import *
from util import *

FILE  = "Chunked_post_tiny.php"
MAGIC = 'Alvaro_alobbs.com'

SCRIPT = """<?php
  echo "Testing Chunked encoded POSTs\n";
  echo $_POST['var'] . "\n";
  echo "The end\n";
?>"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)

        ### TEMPORAL MEASURE ###
        self.disabled = True
        ########################

        self.name = "POST Chunked: Tiny"

        self.request          = "POST /%s HTTP/1.0\r\n" % (FILE) +\
                                "Content-type: application/x-www-form-urlencoded\r\n" +\
                                "Content-length: 0\r\n" +\
                                "Transfer-Encoding: chunked\r\n"
        self.expected_error   = 200
        self.expected_content = [MAGIC]
        self.post             = hex(len(MAGIC)+4)[2:] + "\r\nvar="+MAGIC + "\r\n0\r\n"

    def Prepare (self, www):
        self.WriteFile (www, FILE, 0444, SCRIPT)

    def Precondition (self):
        return os.path.exists (look_for_php())
