from base import *

CONF = """
vserver!default!rule!350!match = directory
vserver!default!rule!350!match!directory = /php1
vserver!default!rule!350!handler = phpcgi
vserver!default!rule!350!handler!interpreter = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name             = "PHP simple, phpcgi"
        self.request          = "GET /php1/simple.php HTTP/1.0\r\n"
        self.conf             = CONF % (look_for_php())
        self.expected_error   = 200
        self.expected_content = "This is PHP"

    def Prepare (self, www):
        self.Mkdir (www, "php1")
        self.WriteFile (www, "php1/simple.php", 0444,
                        "<?php echo 'This'.' is '.'PHP' ?>")

    def Precondition (self):
        return os.path.exists (look_for_php())
