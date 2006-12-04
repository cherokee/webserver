from base import *

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self)
        self.name = "PHP Location header"

        self.request           = "GET /phplocation/redir.php HTTP/1.0\r\n"
        self.expected_error    = 302
        self.expected_content  = "Location: src/login.php"
        self.forbidden_content = "header("

    def Prepare (self, www):
        self.Mkdir (www, "phplocation")
        self.WriteFile (www, "phplocation/redir.php", 0444,
                        '<?php header("Location: src/login.php"); ?>')

    def Precondition (self):
        return os.path.exists (look_for_php())

