from base import *

FILE  = 'special_file_for_179'
MAGIC = 'Alvaro: http://www.alobbs.com/'

CONF = """
vserver!1!rule!1790!match = exists
vserver!1!rule!1790!match!exists = %s
vserver!1!rule!1790!match!final = 1
vserver!1!rule!1790!handler = cgi
""" % (FILE)

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
        self.request           = "GET /%s HTTP/1.0\r\n" % (FILE)
        self.forbidden_content = ['/bin/sh', 'echo']
        self.expected_error    = 200
        self.conf              = CONF

    def Prepare (self, www):
        self.WriteFile (www, FILE, 0555, CGI_BASE % (MAGIC))

