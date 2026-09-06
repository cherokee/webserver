import random
import string
from qa_base import *
from qa_util import *

# This QA test checks:
# http://bugs.cherokee-project.com/504

POST_VAR    = "var"
POST_LENGTH = 1024 * 10
POST_EXTRA  = "\r\n\r\n"
FILENAME    = "post_4extra.php"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Broken post with 4 extra characters"

        self.request          = f"POST /{FILENAME} HTTP/1.1\r\n" +\
                                "Host: localhost\r\n" +\
                                "Content-Type: application/x-www-form-urlencoded\r\n" +\
                                f"Content-length: {int(POST_LENGTH)}\r\n"
        self.expected_error   = 200
        self.post             = POST_VAR + "=" +\
                                letters_random (POST_LENGTH-len(POST_VAR)-1) +\
                                POST_EXTRA

    def Prepare (self, www):
        self.WriteFile (www, FILENAME, 0o444,
                        f"<?php echo $_POST['{POST_VAR}']; ?>")

    def Precondition (self):
        return os.path.exists (look_for_php())
