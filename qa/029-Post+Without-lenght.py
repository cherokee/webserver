from base import *

CONF = """
vserver!default!directory!/post1!handler = phpcgi
vserver!default!directory!/post1!handler!interpreter = %s
vserver!default!directory!/post1!priority = 290
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
