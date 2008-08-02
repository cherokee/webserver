from base import *

CONF = """
vserver!001!rule!290!match = directory
vserver!001!rule!290!match!directory = /post1
vserver!001!rule!290!handler = phpcgi
vserver!001!rule!290!handler!interpreter = %s
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "Post request without length"

        self.conf           = CONF % (look_for_php())
        self.request        = "POST /post1/test.php HTTP/1.0\r\n"
        self.expected_error = 411

    def Prepare (self, www):
        d = self.Mkdir (www, "post1")
        self.WriteFile (d, "test.php", 0444, "<?php echo $_POST; ?>")
