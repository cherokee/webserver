from qa_base import *

DIR       = "extension1"
REQUIRED  = '<a href="http://www.alobbs.com/">Alvaro</a>'
FORBIDDEN = "It shouldn't appear in the text"

CONF = f"""
vserver!1!rule!790!match = directory
vserver!1!rule!790!match!directory = /{DIR}
vserver!1!rule!790!handler = file

vserver!1!rule!791!match = extensions
vserver!1!rule!791!match!extensions = xyz
vserver!1!rule!791!handler = cgi
"""

CGI_BASE = f"""#!/bin/sh
# {FORBIDDEN}
echo "Content-Type: text/plain"
echo
echo '{REQUIRED}'
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Custom extensions"
        self.request           = f"GET /{DIR}/test.xyz HTTP/1.0\r\n"
        self.expected_error    = 200
        self.expected_content  = REQUIRED
        self.forbidden_content = FORBIDDEN
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "test.xyz", 0o755, CGI_BASE)

