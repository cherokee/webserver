from base import *

DIR = "php1"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "PHP simple"
        self.request          = "GET /%s/simple.php HTTP/1.0\r\n" % (DIR)
        self.expected_error   = 200
        self.expected_content = "This is PHP"

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "simple.php", 0444,
                        "<?php echo 'This'.' is '.'PHP' ?>")

    def Precondition (self):
        return os.path.exists (look_for_php())
