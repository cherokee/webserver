from qa_base import *

DIR = "cgi-bin_httpresponse"
MSG = "Page eaten by dog"

CONF = f"""
vserver!1!rule!1980!match = directory
vserver!1!rule!1980!match!directory = /{DIR}
vserver!1!rule!1980!handler = cgi
vserver!1!rule!1980!handler!error_handler = 0
"""

CGI_BASE = f"""#!/bin/sh
echo "Content-Type: text/plain"
echo "HTTP/1.0 404 {MSG}"
echo "Foo: bar"
echo
echo "Sorry, file not found."
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI: HTTP response with reply headers"

        self.request           = f"GET /{DIR}/test HTTP/1.0\r\n"
        self.conf              = CONF
        self.expected_error    = 404
        self.forbidden_content = MSG

    def Prepare (self, www):
        self.Mkdir (www, DIR)
        self.WriteFile (www, f"{DIR}/test", 0o755, CGI_BASE)
