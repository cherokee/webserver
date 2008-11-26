from base import *

FILE1     = 'special_file_for_180'
FILE2     = 'foobar180_file'
MAGIC     = 'Alvaro: http://www.alobbs.com/'
FORBIDDEN = 'This is forbidden string'

CONF = """
vserver!1!rule!1800!match = exists
vserver!1!rule!1800!match!exists = %s
vserver!1!rule!1800!match!final = 1
vserver!1!rule!1800!handler = file

vserver!1!rule!1801!match = exists
vserver!1!rule!1801!match!exists = %s
vserver!1!rule!1801!match!final = 1
vserver!1!rule!1801!handler = cgi
""" % (FILE1, FILE2)

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
        self.name              = "Rule Exists: match 2"
        self.request           = "GET /%s HTTP/1.0\r\n" % (FILE2)
        self.forbidden_content = ['/bin/sh', 'echo', FORBIDDEN]
        self.expected_error    = 200
        self.conf              = CONF

    def Prepare (self, www):
        self.WriteFile (www, FILE1, 0555, FORBIDDEN)
        self.WriteFile (www, FILE2, 0555, CGI_BASE % (MAGIC))

