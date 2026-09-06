from qa_base import *

FILE  = 'special_file_for_179'
MAGIC = 'Alvaro: http://www.alobbs.com/'

CONF = f"""
vserver!1!rule!1790!match = exists
vserver!1!rule!1790!match!exists = {FILE}
vserver!1!rule!1790!match!final = 1
vserver!1!rule!1790!handler = cgi
"""

CGI_BASE = """#!/bin/sh
echo "Content-type: text/html"
echo ""
cat << EOF
%s
EOF
"""

class Test (TestBase):
    def __init__ (self):
        TestBase.__init__ (self, __file__)
        self.name              = "Rule Exists: match"
        self.request           = f"GET /{FILE} HTTP/1.0\r\n"
        self.forbidden_content = ['/bin/sh', 'echo']
        self.expected_error    = 200
        self.conf              = CONF

    def Prepare (self, www):
        self.WriteFile (www, FILE, 0o555, CGI_BASE % (MAGIC))
