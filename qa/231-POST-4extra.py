import random
import string
from base import *
from util import *

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

        self.request          = "POST /%s HTTP/1.1\r\n" %(FILENAME) +\
                                "Host: localhost\r\n" +\
                                "Content-Type: application/x-www-form-urlencoded\r\n" +\
                                "Content-length: %d\r\n" % (POST_LENGTH)
        self.expected_error   = 200
        self.post             = POST_VAR + "=" +\
                                letters_random (POST_LENGTH-len(POST_VAR)-1) +\
                                POST_EXTRA

    def Prepare (self, www):
        self.WriteFile (www, FILENAME, 0444,
                        "<?php echo $_POST['%s']; ?>" %(POST_VAR))

    def Precondition (self):
        return os.path.exists (look_for_php())

