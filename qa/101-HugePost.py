import random
import string
from base import *
from util import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "Huge POST, 1Mb"
        self.expected_error   = 200

    def Prepare (self, www):
        random  = "BEGIN(" + (letters_random (5*1024) * 200) + ")END"
        tmpfile = self.WriteTemp (random)

        self.request          = "POST /Post1Mb.php HTTP/1.0\r\n" +\
                                "Content-type: application/x-www-form-urlencoded\r\n" +\
                                "Content-length: %d\r\n" % (4+len(random))
        self.post             = "var="+random
        self.expected_content = "file:%s" % (tmpfile)

        self.WriteFile (www, "Post1Mb.php", 0444, "<?php echo $_POST['var']; ?>")

    def Precondition (self):
        return os.path.exists (look_for_php())

