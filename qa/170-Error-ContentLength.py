from qa_base import *

DIR       = 'cgi_error_contentlength_1'
ERROR     = 404
ERROR_MSG = letters_random(1234)

CONF = f"""
vserver!1!rule!1700!match = directory
vserver!1!rule!1700!match!directory = /{DIR}
vserver!1!rule!1700!handler = cgi
vserver!1!rule!1700!handler!error_handler = 0
"""

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo "Content-Length: %d"
echo "Status: %s"
echo ""
cat << EOF
%s
EOF
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name = "CGI error_handler: Content-Length"

        self.request           = f"GET /{DIR}/exec.cgi HTTP/1.0\r\n"
        self.expected_error    = ERROR
        self.forbidden_content = f"Content-Length: {len(ERROR_MSG)}"
        self.conf              = CONF

    def Prepare (self, www):
        d = self.Mkdir (www, DIR)
        f = self.WriteFile (d, "exec.cgi", 0o555,
                            CGI_BASE % (len(ERROR_MSG), ERROR, ERROR_MSG))
