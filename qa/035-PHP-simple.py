from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "PHP simple, phpcgi"
        self.request          = "GET /php1/simple.php HTTP/1.0\r\n"
        self.conf             = "Directory /php1 { Handler phpcgi }"
        self.expected_error   = 200
        self.expected_content = "This is PHP"

    def Prepare (self, www):
        self.Mkdir (www, "php1")
        self.WriteFile (www, "php1/simple.php", 0444,
                        "<?php echo 'This'.' is '.'PHP' ?>")

    def Precondition (self):
        return os.path.exists (PHPCGI_PATH)
