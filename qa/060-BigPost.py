import random
import string
from base import *
from util import *

POST_LENGTH = (100*1024)+33


class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Big POST, ~100k"

        self.request          = "POST /Post100k.php HTTP/1.0\r\n" +\
                                "Content-type: application/x-www-form-urlencoded\r\n" +\
                                "Content-length: %d\r\n" % (4+POST_LENGTH)
        self.expected_error   = 200

    def Prepare (self, www):
        random_str  = letters_random (POST_LENGTH)
        tmpfile = self.WriteTemp (random_str)

        self.WriteFile (www, "Post100k.php", 0444,
                        "<?php echo $_POST['var']; ?>")

        self.post             = "var="+random_str
        self.expected_content = "file:%s" % (tmpfile)

    def Precondition (self):
        return os.path.exists (look_for_php())

