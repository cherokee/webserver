from qa_base import *

DIR  = "268_options_php1"
FILE = "file.php"

CONTENT   = "This is inside the file"
FORBIDDEN = "Should not appear"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "OPTIONS: PHP request"
        self.request           = f"OPTIONS /{globals()['DIR']}/{globals()['FILE']} HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = CONTENT
        self.forbidden_content = FORBIDDEN

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, FILE, 0o444,
                        f"<?php /* {globals()['FORBIDDEN']} */ echo '{globals()['CONTENT']}'; ?>")

    def Precondition (self):
        return os.path.exists (look_for_php())
