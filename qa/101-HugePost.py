import random
import string
from qa_base import *
from qa_util import *

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
                                f"Content-length: {int(4 + len(random))}\r\n"
        self.post             = "var="+random
        self.expected_content = f"file:{tmpfile}"

        self.WriteFile (www, "Post1Mb.php", 0o444, "<?php echo $_POST['var']; ?>")

    def Precondition (self):
        return os.path.exists (look_for_php())
