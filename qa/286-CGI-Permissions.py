from qa_base import *

MAGIC = "This is the magic string"

DIR  = "cgi-bin-perms1"
CONF = f"""
vserver!1!rule!2860!match = directory
vserver!1!rule!2860!match!directory = /{globals()['DIR']}
vserver!1!rule!2860!handler = cgi
"""

CGI_BASE = f"""#!/bin/sh
echo "Content-Type: text/plain"
echo
echo "{MAGIC}"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI Wrong permissions"

        self.request        = f"GET /{DIR}/test HTTP/1.0\r\n"
        self.conf           = CONF
        self.expected_error = 403

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test", 0o400, CGI_BASE)
