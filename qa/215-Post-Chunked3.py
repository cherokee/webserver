import random
import string
from qa_base import *
from qa_util import *

FILE      = "Chunked_post_empty.php"
MAGIC     = '<a href="http://www.alobbs.com">Alvaro</a>'
FORBIDDEN = 'foo bar'

SCRIPT = f"""<?php
  echo '{MAGIC}';
  /* {FORBIDDEN} */
?>"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)

        ### TEMPORAL MEASURE ###
        self.disabled = True
        ########################

        self.name = "POST Chunked: Empty"

        self.request           = f"POST /{FILE} HTTP/1.0\r\n" +\
                                 "Content-type: application/x-www-form-urlencoded\r\n" +\
                                 "Content-length: 0\r\n" +\
                                 "Transfer-Encoding: chunked\r\n"
        self.expected_error    = 200
        self.expected_content  = [MAGIC]
        self.forbidden_content = FORBIDDEN
        self.post              = "0\r\n"

    def Prepare (self, www):
        self.WriteFile (www, FILE, 0o444, SCRIPT)

    def Precondition (self):
        return os.path.exists (look_for_php())
