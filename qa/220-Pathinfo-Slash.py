from qa_base import *

DIR = "220_pathinfo_is_slash"

CONF = f"""
vserver!1!rule!2201!match = extensions
vserver!1!rule!2201!match!extensions = xx220xx
vserver!1!rule!2201!handler = cgi

vserver!1!rule!2200!match = directory
vserver!1!rule!2200!match!directory = /{DIR}
vserver!1!rule!2200!handler = file
"""

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo ""
echo "PATH_INFO: ($PATH_INFO)"
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Pathinfo is /"
        self.request           = f"GET /{DIR}/index.xx220xx/ HTTP/1.0\r\n"
        self.conf              = CONF
        self.expected_error    = 200
        self.expected_content  = "PATH_INFO: (/)"

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        self.WriteFile (d, "index.xx220xx", 0o755, CGI_BASE)
