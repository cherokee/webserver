from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Post request without length"

        self.conf           = "Directory /post1 { Handler phpcgi }"
        self.request        = "POST /post1/test.php HTTP/1.0\r\n"
        self.expected_error = 411

    def Prepare (self, www):
        self.Mkdir (www, "post1")
        self.WriteFile (www, "post1/test.php", 0444, "<?php echo $_POST; ?>")
                        

