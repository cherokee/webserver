from qa_base import *

HEADER_VAL="Cherokee_supports_If-None-Match"

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name             = "PHP: If-None-Match header"
        self.request          = "GET /if_none_match1/test.php HTTP/1.0\r\n" + \
                                f"If-None-Match: {HEADER_VAL}\r\n"
        self.expected_error   = 304

    def Prepare (self, www):
        d = self.Mkdir (www, "if_none_match1")
        self.WriteFile (d, "test.php", 0o444, """<?php
           if (isset($_SERVER['HTTP_IF_NONE_MATCH']) && $_SERVER['HTTP_IF_NONE_MATCH'] == '{}') {{
              header('HTTP/1.0 304 Not Modified');
	         exit();
           }}
           echo 'This should not happen\n';
        ?>""".format(HEADER_VAL))

    def Precondition (self):
        return os.path.exists (look_for_php())
