import random
import string
from base import *
from util import *

FILE        = "Chunked_post1.php"
POST_LENGTH = (100*1024)+35
MAGIC       = letters_random (POST_LENGTH)

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

        self.name = "POST Chunked: ~100k"

        self.request          = "POST /%s HTTP/1.0\r\n" % (FILE) +\
                                "Content-type: application/x-www-form-urlencoded\r\n" +\
                                "Content-length: 0\r\n" +\
                                "Transfer-Encoding: chunked\r\n"
        self.expected_error   = 200

    def Prepare (self, www):
        tmpfile = self.WriteTemp (MAGIC)
        self.WriteFile (www, FILE, 0444, SCRIPT)

        self.post             = chunk_encode("var=" + MAGIC)
        self.expected_content = "file:%s" % (tmpfile)

    def Precondition (self):
        return os.path.exists (look_for_php())
