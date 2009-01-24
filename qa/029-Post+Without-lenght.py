from base import *

DIR = "post1"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name           = "Post request without length"
        self.request        = "POST /%s/test.php HTTP/1.0\r\n" % (DIR)
        self.expected_error = 411

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test.php", 0444, "<?php echo $_POST; ?>")
